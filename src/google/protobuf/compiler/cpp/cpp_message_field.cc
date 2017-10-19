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

#include <google/protobuf/compiler/cpp/cpp_message_field.h>
#include <google/protobuf/compiler/cpp/cpp_helpers.h>
#include <google/protobuf/io/printer.h>

#include <google/protobuf/stubs/strutil.h>

namespace google {
namespace protobuf {
namespace compiler {
namespace cpp {

namespace {

// When we are generating code for implicit weak fields, we need to insert some
// additional casts. These functions return the casted expression if
// implicit_weak_field is true but otherwise return the original expression.
// Ordinarily a static_cast is enough to cast google::protobuf::MessageLite* to a class
// deriving from it, but we need a reinterpret_cast in cases where the generated
// message is forward-declared but its full definition is not visible.
string StaticCast(const string& type, const string& expression,
                  bool implicit_weak_field) {
  if (implicit_weak_field) {
    return "static_cast< " + type + " >(" + expression + ")";
  } else {
    return expression;
  }
}

string ReinterpretCast(const string& type, const string& expression,
                       bool implicit_weak_field) {
  if (implicit_weak_field) {
    return "reinterpret_cast< " + type + " >(" + expression + ")";
  } else {
    return expression;
  }
}

void SetMessageVariables(const FieldDescriptor* descriptor,
                         std::map<string, string>* variables,
                         const Options& options) {
  SetCommonFieldVariables(descriptor, variables, options);
  (*variables)["type"] = FieldMessageTypeName(descriptor);
  (*variables)["casted_member"] =
      ReinterpretCast((*variables)["type"] + "*", (*variables)["name"] + "_",
                      IsImplicitWeakField(descriptor, options));
  (*variables)["type_default_instance"] =
      DefaultInstanceName(descriptor->message_type());
  (*variables)["type_reference_function"] =
      ReferenceFunctionName(descriptor->message_type());
  if (descriptor->options().weak() || !descriptor->containing_oneof()) {
    (*variables)["non_null_ptr_to_name"] =
        StrCat("this->", (*variables)["name"], "_");
  }
  (*variables)["stream_writer"] =
      (*variables)["declared_type"] +
      (HasFastArraySerialization(descriptor->message_type()->file(), options)
           ? "MaybeToArray"
           : "");
  // NOTE: Escaped here to unblock proto1->proto2 migration.
  // TODO(liujisi): Extend this to apply for other conflicting methods.
  (*variables)["release_name"] =
      SafeFunctionName(descriptor->containing_type(),
                       descriptor, "release_");
  (*variables)["full_name"] = descriptor->full_name();
  if (options.proto_h && IsFieldDependent(descriptor)) {
    (*variables)["dependent_type"] = "T::" + DependentTypeName(descriptor);
    (*variables)["dependent_typename"] =
        "typename T::" + DependentTypeName(descriptor);
  } else {
    (*variables)["dependent_type"] = FieldMessageTypeName(descriptor);
    (*variables)["dependent_typename"] = FieldMessageTypeName(descriptor);
  }
}

}  // namespace

// ===================================================================

MessageFieldGenerator::MessageFieldGenerator(const FieldDescriptor* descriptor,
                                             const Options& options)
    : FieldGenerator(options),
      descriptor_(descriptor),
      dependent_field_(options.proto_h && IsFieldDependent(descriptor)),
      implicit_weak_field_(IsImplicitWeakField(descriptor, options)) {
  SetMessageVariables(descriptor, &variables_, options);
}

MessageFieldGenerator::~MessageFieldGenerator() {}

void MessageFieldGenerator::
GeneratePrivateMembers(io::Printer* printer) const {
  if (implicit_weak_field_) {
    printer->Print(variables_, "google::protobuf::MessageLite* $name$_;\n");
  } else {
    printer->Print(variables_, "$type$* $name$_;\n");
  }
}

void MessageFieldGenerator::
GenerateDependentAccessorDeclarations(io::Printer* printer) const {
  if (!dependent_field_) {
    return;
  }
  printer->Print(variables_,
                 "$deprecated_attr$$type$* ${$mutable_$name$$}$();\n");
  printer->Annotate("{", "}", descriptor_);
}

void MessageFieldGenerator::
GenerateAccessorDeclarations(io::Printer* printer) const {
  if (SupportsArenas(descriptor_) && !implicit_weak_field_) {
    printer->Print(variables_,
       "private:\n"
       "void _slow_mutable_$name$();\n"
       "public:\n");
  }
  if (implicit_weak_field_) {
    // These private accessors are used by MergeFrom and
    // MergePartialFromCodedStream, and their purpose is to provide access to
    // the field without creating a strong dependency on the message type.
    printer->Print(variables_,
       "private:\n"
       "const google::protobuf::MessageLite& _internal_$name$() const;\n"
       "google::protobuf::MessageLite* _internal_mutable_$name$();\n"
       "public:\n");
  }
  printer->Print(variables_,
      "$deprecated_attr$const $type$& $name$() const;\n");
  printer->Annotate("name", descriptor_);
  printer->Print(variables_, "$deprecated_attr$$type$* $release_name$();\n");
  printer->Annotate("release_name", descriptor_);
  if (!dependent_field_) {
    printer->Print(variables_,
                   "$deprecated_attr$$type$* ${$mutable_$name$$}$();\n");
    printer->Annotate("{", "}", descriptor_);
  }
  printer->Print(variables_,
                 "$deprecated_attr$void ${$set_allocated_$name$$}$"
                 "($type$* $name$);\n");
  printer->Annotate("{", "}", descriptor_);
  if (SupportsArenas(descriptor_)) {
    printer->Print(variables_,
                   "$deprecated_attr$void "
                   "${$unsafe_arena_set_allocated_$name$$}$(\n"
                   "    $type$* $name$);\n");
    printer->Annotate("{", "}", descriptor_);
    printer->Print(
        variables_,
        "$deprecated_attr$$type$* ${$unsafe_arena_release_$name$$}$();\n");
    printer->Annotate("{", "}", descriptor_);
  }
}

void MessageFieldGenerator::GenerateNonInlineAccessorDefinitions(
    io::Printer* printer) const {
  if (implicit_weak_field_) {
    printer->Print(variables_,
      "const google::protobuf::MessageLite& $classname$::_internal_$name$() const {\n"
      "  if ($name$_ != NULL) {\n"
      "    return *$name$_;\n"
      "  } else if (&$type_default_instance$ != NULL) {\n"
      "    return *reinterpret_cast<const google::protobuf::MessageLite*>(\n"
      "        &$type_default_instance$);\n"
      "  } else {\n"
      "    return *reinterpret_cast<const google::protobuf::MessageLite*>(\n"
      "        &::google::protobuf::internal::implicit_weak_message_default_instance);\n"
      "  }\n"
      "}\n");
  }
  if (SupportsArenas(descriptor_)) {
    if (implicit_weak_field_) {
      printer->Print(variables_,
        "google::protobuf::MessageLite* $classname$::_internal_mutable_$name$() {\n"
        "  $set_hasbit$\n"
        "  if ($name$_ == NULL) {\n"
        "    if (&$type_default_instance$ == NULL) {\n"
        "      $name$_ = ::google::protobuf::Arena::CreateMessage<\n"
        "          ::google::protobuf::internal::ImplicitWeakMessage>(\n"
        "              GetArenaNoVirtual());\n"
        "    } else {\n"
        "      $name$_ = reinterpret_cast<const google::protobuf::MessageLite*>(\n"
        "          &$type_default_instance$)->New(GetArenaNoVirtual());\n"
        "    }\n"
        "  }\n"
        "  return $name$_;\n");
    } else {
      printer->Print(variables_,
        "void $classname$::_slow_mutable_$name$() {\n");
      if (SupportsArenas(descriptor_->message_type())) {
        printer->Print(variables_,
          "  $name$_ = ::google::protobuf::Arena::CreateMessage< $type$ >(\n"
          "      GetArenaNoVirtual());\n");
      } else {
        printer->Print(variables_,
          "  $name$_ = ::google::protobuf::Arena::Create< $type$ >(\n"
          "      GetArenaNoVirtual());\n");
      }
    }
    printer->Print(variables_,
      "}\n");

    printer->Print(variables_,
      "void $classname$::unsafe_arena_set_allocated_$name$(\n"
      "    $type$* $name$) {\n"
      // If we're not on an arena, free whatever we were holding before.
      // (If we are on arena, we can just forget the earlier pointer.)
      "  if (GetArenaNoVirtual() == NULL) {\n"
      "    delete $name$_;\n"
      "  }\n"
      "  $name$_ = $name$;\n"
      "  if ($name$) {\n"
      "    $set_hasbit$\n"
      "  } else {\n"
      "    $clear_hasbit$\n"
      "  }\n"
      "  // @@protoc_insertion_point(field_unsafe_arena_set_allocated"
      ":$full_name$)\n"
      "}\n");
  } else if (implicit_weak_field_) {
    printer->Print(variables_,
        "google::protobuf::MessageLite* $classname$::_internal_mutable_$name$() {\n"
        "  $set_hasbit$\n"
        "  if ($name$_ == NULL) {\n"
        "    if (&$type_default_instance$ == NULL) {\n"
        "      $name$_ = new ::google::protobuf::internal::ImplicitWeakMessage;\n"
        "    } else {\n"
        "      $name$_ = reinterpret_cast<const google::protobuf::MessageLite*>(\n"
        "          &$type_default_instance$)->New();\n"
        "    }\n"
        "  }\n"
        "  return $name$_;\n"
        "}\n");
  }
}

void MessageFieldGenerator::
GenerateDependentInlineAccessorDefinitions(io::Printer* printer) const {
  if (!dependent_field_) {
    return;
  }

  std::map<string, string> variables(variables_);
  // For the CRTP base class, all mutation methods are dependent, and so
  // they must be in the header.
  variables["dependent_classname"] =
      DependentBaseClassTemplateName(descriptor_->containing_type()) + "<T>";
  variables["this_message"] = DependentBaseDownCast();
  variables["casted_reference"] =
      ReinterpretCast(variables["dependent_typename"] + "*&",
                      variables["this_message"] + variables["name"] + "_",
                      implicit_weak_field_);
  if (!variables["set_hasbit"].empty()) {
    variables["set_hasbit"] =
        variables["this_message"] + variables["set_hasbit"];
  }
  if (!variables["clear_hasbit"].empty()) {
    variables["clear_hasbit"] =
        variables["this_message"] + variables["clear_hasbit"];
  }

  if (SupportsArenas(descriptor_)) {
    printer->Print(variables,
      "template <class T>\n"
      "inline $type$* $dependent_classname$::mutable_$name$() {\n");
    if (implicit_weak_field_) {
      printer->Print(variables, "  $type_reference_function$();\n");
    }
    printer->Print(variables,
      "  $set_hasbit$\n"
      "  $dependent_typename$*& $name$_ = $casted_reference$;\n"
      "  if ($name$_ == NULL) {\n");
    if (implicit_weak_field_) {
      printer->Print(variables,
          "    $name$_ = reinterpret_cast<$dependent_typename$*>(\n"
          "        reinterpret_cast<const google::protobuf::MessageLite*>(\n"
          "        &$type_default_instance$)->New(\n"
          "        $this_message$GetArenaNoVirtual()));\n");
    } else {
      printer->Print(variables,
        "    $this_message$_slow_mutable_$name$();\n");
    }
    printer->Print(variables,
      "  }\n"
      "  // @@protoc_insertion_point(field_mutable:$full_name$)\n"
      "  return $name$_;\n"
      "}\n");
  } else {
    printer->Print(variables,
      "template <class T>\n"
      "inline $type$* $dependent_classname$::mutable_$name$() {\n"
      "  $set_hasbit$\n"
      "  $dependent_typename$*& $name$_ = $casted_reference$;\n"
      "  if ($name$_ == NULL) {\n"
      "    $name$_ = new $dependent_typename$;\n"
      "  }\n"
      "  // @@protoc_insertion_point(field_mutable:$full_name$)\n"
      "  return $name$_;\n"
      "}\n");
  }
}

void MessageFieldGenerator::
GenerateInlineAccessorDefinitions(io::Printer* printer) const {
  std::map<string, string> variables(variables_);
  variables["const_member"] = ReinterpretCast(
      "const " + variables["type"] + "*", variables["name"] + "_",
      implicit_weak_field_);
  printer->Print(variables,
    "inline const $type$& $classname$::$name$() const {\n");
  if (implicit_weak_field_) {
    printer->Print(variables, "  $type_reference_function$();\n");
  }
  printer->Print(variables,
    "  const $type$* p = $const_member$;\n"
    "  // @@protoc_insertion_point(field_get:$full_name$)\n"
    "  return p != NULL ? *p : *reinterpret_cast<const $type$*>(\n"
    "      &$type_default_instance$);\n"
    "}\n");

  printer->Print(variables,
    "inline $type$* $classname$::$release_name$() {\n"
    "  // @@protoc_insertion_point(field_release:$full_name$)\n");
  if (implicit_weak_field_) {
    printer->Print(variables, "  $type_reference_function$();\n");
  }
  printer->Print(variables,
    "  $clear_hasbit$\n"
    "  $type$* temp = $casted_member$;\n");
  if (SupportsArenas(descriptor_)) {
    printer->Print(variables,
      "  if (GetArenaNoVirtual() != NULL) {\n"
      "    temp = ::google::protobuf::internal::DuplicateIfNonNull(temp, NULL);\n"
      "  }\n");
  }
  printer->Print(variables,
    "  $name$_ = NULL;\n"
    "  return temp;\n"
    "}\n");

  if (SupportsArenas(descriptor_)) {
    printer->Print(variables,
      "inline $type$* $classname$::unsafe_arena_release_$name$() {\n"
      "  // @@protoc_insertion_point("
      "field_unsafe_arena_release:$full_name$)\n");
    if (implicit_weak_field_) {
      printer->Print(variables, "  $type_reference_function$();\n");
    }
    printer->Print(variables,
      "  $clear_hasbit$\n"
      "  $type$* temp = $casted_member$;\n"
      "  $name$_ = NULL;\n"
      "  return temp;\n"
      "}\n");
  }

  if (!dependent_field_) {
    if (SupportsArenas(descriptor_)) {
      printer->Print(variables,
        "inline $type$* $classname$::mutable_$name$() {\n"
        "  $set_hasbit$\n"
        "  if ($name$_ == NULL) {\n");
      if (implicit_weak_field_) {
        printer->Print(variables,
          "    _internal_mutable_$name$();\n");
      } else {
        printer->Print(variables,
          "    _slow_mutable_$name$();\n");
      }
      printer->Print(variables,
        "  }\n"
        "  // @@protoc_insertion_point(field_mutable:$full_name$)\n"
        "  return $casted_member$;\n"
        "}\n");
    } else {
      printer->Print(variables,
        "inline $type$* $classname$::mutable_$name$() {\n"
        "  $set_hasbit$\n"
        "  if ($name$_ == NULL) {\n"
        "    $name$_ = new $type$;\n"
        "  }\n"
        "  // @@protoc_insertion_point(field_mutable:$full_name$)\n"
        "  return $casted_member$;\n"
        "}\n");
    }
  }


  // We handle the most common case inline, and delegate less common cases to
  // the slow fallback function.
  printer->Print(variables,
    "inline void $classname$::set_allocated_$name$($type$* $name$) {\n"
    "  ::google::protobuf::Arena* message_arena = GetArenaNoVirtual();\n");
  printer->Print(variables,
    "  if (message_arena == NULL) {\n");
  if (IsCrossFileMessage(descriptor_)) {
    printer->Print(variables,
      "    delete reinterpret_cast< ::google::protobuf::MessageLite*>($name$_);\n");
  } else {
    printer->Print(variables,
      "    delete $name$_;\n");
  }
  printer->Print(variables,
    "  }\n"
    "  if ($name$) {\n");
  if (SupportsArenas(descriptor_->message_type()) &&
      IsCrossFileMessage(descriptor_)) {
    // We have to read the arena through the virtual method, because the type
    // isn't defined in this file.
    printer->Print(variables,
      "    ::google::protobuf::Arena* submessage_arena =\n"
      "      reinterpret_cast< ::google::protobuf::MessageLite*>($name$)->GetArena();\n");
  } else if (!SupportsArenas(descriptor_->message_type())) {
    printer->Print(variables,
      "    ::google::protobuf::Arena* submessage_arena = NULL;\n");
  } else {
    printer->Print(variables,
      "    ::google::protobuf::Arena* submessage_arena =\n"
      "      ::google::protobuf::Arena::GetArena($name$);\n");
  }
  printer->Print(variables,
    "    if (message_arena != submessage_arena) {\n"
    "      $name$ = ::google::protobuf::internal::GetOwnedMessage(\n"
    "          message_arena, $name$, submessage_arena);\n"
    "    }\n"
    "    $set_hasbit$\n"
    "  } else {\n"
    "    $clear_hasbit$\n"
    "  }\n");
  if (implicit_weak_field_) {
    printer->Print(variables,
      "  $name$_ = reinterpret_cast<MessageLite*>($name$);\n");
  } else {
    printer->Print(variables,
      "  $name$_ = $name$;\n");
  }
  printer->Print(variables,
    "  // @@protoc_insertion_point(field_set_allocated:$full_name$)\n"
    "}\n");
}

void MessageFieldGenerator::
GenerateClearingCode(io::Printer* printer) const {
  if (!HasFieldPresence(descriptor_->file())) {
    // If we don't have has-bits, message presence is indicated only by ptr !=
    // NULL. Thus on clear, we need to delete the object.
    printer->Print(variables_,
      "if (GetArenaNoVirtual() == NULL && $name$_ != NULL) {\n"
      "  delete $name$_;\n"
      "}\n"
      "$name$_ = NULL;\n");
  } else {
    printer->Print(variables_,
      "if ($name$_ != NULL) $name$_->Clear();\n");
  }
}

void MessageFieldGenerator::
GenerateMessageClearingCode(io::Printer* printer) const {
  if (!HasFieldPresence(descriptor_->file())) {
    // If we don't have has-bits, message presence is indicated only by ptr !=
    // NULL. Thus on clear, we need to delete the object.
    printer->Print(variables_,
      "if (GetArenaNoVirtual() == NULL && $name$_ != NULL) {\n"
      "  delete $name$_;\n"
      "}\n"
      "$name$_ = NULL;\n");
  } else {
    printer->Print(variables_,
      "GOOGLE_DCHECK($name$_ != NULL);\n"
      "$name$_->Clear();\n");
  }
}

void MessageFieldGenerator::
GenerateMergingCode(io::Printer* printer) const {
  if (implicit_weak_field_) {
    printer->Print(variables_,
      "_internal_mutable_$name$()->CheckTypeAndMergeFrom(\n"
      "    from._internal_$name$());\n");
  } else {
    printer->Print(variables_,
      "mutable_$name$()->$type$::MergeFrom(from.$name$());\n");
  }
}

void MessageFieldGenerator::
GenerateSwappingCode(io::Printer* printer) const {
  printer->Print(variables_, "swap($name$_, other->$name$_);\n");
}

void MessageFieldGenerator::
GenerateDestructorCode(io::Printer* printer) const {
  // TODO(gerbens) Remove this when we don't need to destruct default instances.
  // In google3 a default instance will never get deleted so we don't need to
  // worry about that but in opensource protobuf default instances are deleted
  // in shutdown process and we need to take special care when handling them.
  printer->Print(variables_,
    "if (this != internal_default_instance()) ");
  printer->Print(variables_, "delete $name$_;\n");
}

void MessageFieldGenerator::
GenerateConstructorCode(io::Printer* printer) const {
  printer->Print(variables_, "$name$_ = NULL;\n");
}

void MessageFieldGenerator::
GenerateCopyConstructorCode(io::Printer* printer) const {
  // For non-Arena enabled messages, everything always goes on the heap.
  //
  // For Arena enabled messages, the logic is a bit more convoluted.
  //
  // In the copy constructor, we call InternalMetadataWithArena::MergeFrom,
  // which does *not* copy the Arena pointer.  In the generated MergeFrom
  // (see MessageFieldGenerator::GenerateMergingCode), we:
  // -> copy the has bits (but this is done in bulk by a memcpy in the copy
  //    constructor)
  // -> check whether the destination field pointer is NULL (it will be, since
  //    we're initializing it and would have called SharedCtor) and if so:
  // -> call _slow_mutable_$name$(), which calls either
  //    ::google::protobuf::Arena::CreateMessage<>(GetArenaNoVirtual()), or
  //    ::google::protobuf::Arena::Create<>(GetArenaNoVirtual())
  //
  // At this point, GetArenaNoVirtual returns NULL since the Arena pointer
  // wasn't copied, so both of these methods allocate the submessage on the
  // heap.

  string new_expression = (implicit_weak_field_ ? "from.$name$_->New()"
                                                : "new $type$(*from.$name$_)");
  string output =
      "if (from.has_$name$()) {\n"
      "  $name$_ = " + new_expression + ";\n"
      "} else {\n"
      "  $name$_ = NULL;\n"
      "}\n";
  printer->Print(variables_, output.c_str());
}

void MessageFieldGenerator::
GenerateMergeFromCodedStream(io::Printer* printer) const {
  if (implicit_weak_field_) {
    printer->Print(variables_,
      "DO_(::google::protobuf::internal::WireFormatLite::ReadMessage(\n"
      "     input, _internal_mutable_$name$()));\n");
  } else if (descriptor_->type() == FieldDescriptor::TYPE_MESSAGE) {
    printer->Print(variables_,
      "DO_(::google::protobuf::internal::WireFormatLite::ReadMessage(\n"
      "     input, mutable_$name$()));\n");
  } else {
    printer->Print(variables_,
      "DO_(::google::protobuf::internal::WireFormatLite::ReadGroup(\n"
      "      $number$, input, mutable_$name$()));\n");
  }
}

void MessageFieldGenerator::
GenerateSerializeWithCachedSizes(io::Printer* printer) const {
  printer->Print(variables_,
    "::google::protobuf::internal::WireFormatLite::Write$stream_writer$(\n"
    "  $number$, *$non_null_ptr_to_name$, output);\n");
}

void MessageFieldGenerator::
GenerateSerializeWithCachedSizesToArray(io::Printer* printer) const {
  printer->Print(variables_,
    "target = ::google::protobuf::internal::WireFormatLite::\n"
    "  InternalWrite$declared_type$ToArray(\n"
    "    $number$, *$non_null_ptr_to_name$, deterministic, target);\n");
}

void MessageFieldGenerator::
GenerateByteSize(io::Printer* printer) const {
  printer->Print(variables_,
    "total_size += $tag_size$ +\n"
    "  ::google::protobuf::internal::WireFormatLite::$declared_type$Size(\n"
    "    *$non_null_ptr_to_name$);\n");
}

// ===================================================================

MessageOneofFieldGenerator::
MessageOneofFieldGenerator(const FieldDescriptor* descriptor,
                           const Options& options)
  : MessageFieldGenerator(descriptor, options),
    dependent_base_(options.proto_h) {
  SetCommonOneofFieldVariables(descriptor, &variables_);
}

MessageOneofFieldGenerator::~MessageOneofFieldGenerator() {}

void MessageOneofFieldGenerator::GenerateNonInlineAccessorDefinitions(
    io::Printer* printer) const {
  printer->Print(variables_,
    "void $classname$::set_allocated_$name$($type$* $name$) {\n"
    "  ::google::protobuf::Arena* message_arena = GetArenaNoVirtual();\n"
    "  clear_$oneof_name$();\n"
    "  if ($name$) {\n");
  if (SupportsArenas(descriptor_->message_type()) &&
      descriptor_->file() != descriptor_->message_type()->file()) {
    // We have to read the arena through the virtual method, because the type
    // isn't defined in this file.
    printer->Print(variables_,
      "    ::google::protobuf::Arena* submessage_arena =\n"
      "      reinterpret_cast< ::google::protobuf::MessageLite*>($name$)->GetArena();\n");
  } else if (!SupportsArenas(descriptor_->message_type())) {
    printer->Print(variables_,
      "    ::google::protobuf::Arena* submessage_arena = NULL;\n");
  } else {
    printer->Print(variables_,
      "    ::google::protobuf::Arena* submessage_arena =\n"
      "      ::google::protobuf::Arena::GetArena($name$);\n");
  }
  printer->Print(variables_,
    "    if (message_arena != submessage_arena) {\n"
    "      $name$ = ::google::protobuf::internal::GetOwnedMessage(\n"
    "          message_arena, $name$, submessage_arena);\n"
    "    }\n"
    "    set_has_$name$();\n"
    "    $oneof_prefix$$name$_ = $name$;\n"
    "  }\n"
    "  // @@protoc_insertion_point(field_set_allocated:$full_name$)\n"
    "}\n");
}

void MessageOneofFieldGenerator::
GenerateDependentInlineAccessorDefinitions(io::Printer* printer) const {
  // For the CRTP base class, all mutation methods are dependent, and so
  // they must be in the header.
  if (!dependent_base_) {
    return;
  }
  std::map<string, string> variables(variables_);
  variables["dependent_classname"] =
      DependentBaseClassTemplateName(descriptor_->containing_type()) + "<T>";
  variables["this_message"] = "reinterpret_cast<T*>(this)->";
  // Const message access is needed for the dependent getter.
  variables["this_const_message"] = "reinterpret_cast<const T*>(this)->";
  variables["tmpl"] = "template <class T>\n";
  variables["field_member"] = variables["this_message"] +
                              variables["oneof_prefix"] + variables["name"] +
                              "_";
  InternalGenerateInlineAccessorDefinitions(variables, printer);
}

void MessageOneofFieldGenerator::
GenerateInlineAccessorDefinitions(io::Printer* printer) const {

  std::map<string, string> variables(variables_);
  variables["dependent_classname"] = variables["classname"];
  variables["this_message"] = "";
  variables["this_const_message"] = "";
  variables["tmpl"] = "";
  variables["field_member"] =
      variables["oneof_prefix"] + variables["name"] + "_";
  variables["dependent_type"] = variables["type"];

  printer->Print(variables,
    "inline $type$* $classname$::$release_name$() {\n"
    "  // @@protoc_insertion_point(field_release:$full_name$)\n"
    "  if ($this_message$has_$name$()) {\n"
    "    $this_message$clear_has_$oneof_name$();\n"
    "      $type$* temp = $field_member$;\n");
  if (SupportsArenas(descriptor_)) {
    printer->Print(variables,
      "    if ($this_message$GetArenaNoVirtual() != NULL) {\n"
      "      temp = ::google::protobuf::internal::DuplicateIfNonNull(temp, NULL);\n"
      "    }\n");
  }
  printer->Print(variables,
    "    $field_member$ = NULL;\n"
    "    return temp;\n"
    "  } else {\n"
    "    return NULL;\n"
    "  }\n"
    "}\n");

  printer->Print(variables,
    "inline const $type$& $classname$::$name$() const {\n"
    "  // @@protoc_insertion_point(field_get:$full_name$)\n"
    "  return $this_const_message$has_$name$()\n"
    "      ? *$this_const_message$$oneof_prefix$$name$_\n"
    "      : *reinterpret_cast< $type$*>(&$type_default_instance$);\n"
    "}\n");

  if (SupportsArenas(descriptor_)) {
    printer->Print(variables,
      "inline $type$* $dependent_classname$::unsafe_arena_release_$name$() {\n"
      "  // @@protoc_insertion_point(field_unsafe_arena_release"
      ":$full_name$)\n"
      "  if ($this_message$has_$name$()) {\n"
      "    $this_message$clear_has_$oneof_name$();\n"
      "    $type$* temp = $this_message$$oneof_prefix$$name$_;\n"
      "    $this_message$$oneof_prefix$$name$_ = NULL;\n"
      "    return temp;\n"
      "  } else {\n"
      "    return NULL;\n"
      "  }\n"
      "}\n"
      "inline void $classname$::unsafe_arena_set_allocated_$name$"
      "($type$* $name$) {\n"
      // We rely on the oneof clear method to free the earlier contents of this
      // oneof. We can directly use the pointer we're given to set the new
      // value.
      "  clear_$oneof_name$();\n"
      "  if ($name$) {\n"
      "    set_has_$name$();\n"
      "    $oneof_prefix$$name$_ = $name$;\n"
      "  }\n"
      "  // @@protoc_insertion_point(field_unsafe_arena_set_allocated:"
      "$full_name$)\n"
      "}\n");
  }

  if (dependent_base_) {
    return;
  }

  InternalGenerateInlineAccessorDefinitions(variables, printer);
}

void MessageOneofFieldGenerator::InternalGenerateInlineAccessorDefinitions(
    const std::map<string, string>& variables, io::Printer* printer) const {
  if (SupportsArenas(descriptor_)) {
    printer->Print(variables,
      "$tmpl$"
      "inline $type$* $dependent_classname$::mutable_$name$() {\n"
      "  if (!$this_message$has_$name$()) {\n"
      "    $this_message$clear_$oneof_name$();\n"
      "    $this_message$set_has_$name$();\n");
    if (SupportsArenas(descriptor_->message_type())) {
      printer->Print(variables,
         "    $field_member$ = \n"
         "      ::google::protobuf::Arena::CreateMessage< $dependent_typename$ >(\n"
         "      $this_message$GetArenaNoVirtual());\n");
    } else {
      printer->Print(variables,
         "    $this_message$$oneof_prefix$$name$_ = \n"
         "      ::google::protobuf::Arena::Create< $dependent_typename$ >(\n"
         "      $this_message$GetArenaNoVirtual());\n");
    }
    printer->Print(variables,
      "  }\n"
      "  // @@protoc_insertion_point(field_mutable:$full_name$)\n"
      "  return $field_member$;\n"
      "}\n");
  } else {
    printer->Print(variables,
      "$tmpl$"
      "inline $type$* $dependent_classname$::mutable_$name$() {\n"
      "  if (!$this_message$has_$name$()) {\n"
      "    $this_message$clear_$oneof_name$();\n"
      "    $this_message$set_has_$name$();\n"
      "    $field_member$ = new $dependent_typename$;\n"
      "  }\n"
      "  // @@protoc_insertion_point(field_mutable:$full_name$)\n"
      "  return $field_member$;\n"
      "}\n");
  }
}

void MessageOneofFieldGenerator::
GenerateClearingCode(io::Printer* printer) const {
  if (SupportsArenas(descriptor_)) {
    printer->Print(variables_,
      "if (GetArenaNoVirtual() == NULL) {\n"
      "  delete $oneof_prefix$$name$_;\n"
      "}\n");
  } else {
    printer->Print(variables_,
      "delete $oneof_prefix$$name$_;\n");
  }
}

void MessageOneofFieldGenerator::
GenerateMessageClearingCode(io::Printer* printer) const {
  GenerateClearingCode(printer);
}

void MessageOneofFieldGenerator::
GenerateSwappingCode(io::Printer* printer) const {
  // Don't print any swapping code. Swapping the union will swap this field.
}

void MessageOneofFieldGenerator::
GenerateDestructorCode(io::Printer* printer) const {
  // We inherit from MessageFieldGenerator, so we need to override the default
  // behavior.
}

void MessageOneofFieldGenerator::
GenerateConstructorCode(io::Printer* printer) const {
  // Don't print any constructor code. The field is in a union. We allocate
  // space only when this field is used.
}

// ===================================================================

RepeatedMessageFieldGenerator::RepeatedMessageFieldGenerator(
    const FieldDescriptor* descriptor, const Options& options)
    : FieldGenerator(options),
      descriptor_(descriptor),
      dependent_field_(options.proto_h && IsFieldDependent(descriptor)),
      dependent_getter_(dependent_field_ && options.safe_boundary_check) {
  SetMessageVariables(descriptor, &variables_, options);
}

RepeatedMessageFieldGenerator::~RepeatedMessageFieldGenerator() {}

void RepeatedMessageFieldGenerator::
GeneratePrivateMembers(io::Printer* printer) const {
  printer->Print(variables_,
    "::google::protobuf::RepeatedPtrField< $type$ > $name$_;\n");
}

void RepeatedMessageFieldGenerator::
InternalGenerateTypeDependentAccessorDeclarations(io::Printer* printer) const {
  printer->Print(variables_,
                 "$deprecated_attr$$type$* ${$mutable_$name$$}$(int index);\n");
  printer->Annotate("{", "}", descriptor_);
  printer->Print(variables_, "$deprecated_attr$$type$* ${$add_$name$$}$();\n");
  printer->Annotate("{", "}", descriptor_);
  if (dependent_getter_) {
    printer->Print(variables_,
      "$deprecated_attr$const ::google::protobuf::RepeatedPtrField< $type$ >&\n"
      "    $name$() const;\n");
    printer->Annotate("name", descriptor_);
  }
  printer->Print(variables_,
                 "$deprecated_attr$::google::protobuf::RepeatedPtrField< $type$ >*\n"
                 "    ${$mutable_$name$$}$();\n");
  printer->Annotate("{", "}", descriptor_);
}

void RepeatedMessageFieldGenerator::
GenerateDependentAccessorDeclarations(io::Printer* printer) const {
  if (dependent_getter_) {
    printer->Print(variables_,
      "$deprecated_attr$const $type$& $name$(int index) const;\n");
    printer->Annotate("name", descriptor_);
  }
  if (dependent_field_) {
    InternalGenerateTypeDependentAccessorDeclarations(printer);
  }
}

void RepeatedMessageFieldGenerator::
GenerateAccessorDeclarations(io::Printer* printer) const {
  if (!dependent_getter_) {
    printer->Print(variables_,
      "$deprecated_attr$const $type$& $name$(int index) const;\n");
    printer->Annotate("name", descriptor_);
  }
  if (!dependent_field_) {
    InternalGenerateTypeDependentAccessorDeclarations(printer);
  }
  if (!dependent_getter_) {
    printer->Print(variables_,
      "$deprecated_attr$const ::google::protobuf::RepeatedPtrField< $type$ >&\n"
      "    $name$() const;\n");
    printer->Annotate("name", descriptor_);
  }
}

void RepeatedMessageFieldGenerator::
GenerateDependentInlineAccessorDefinitions(io::Printer* printer) const {
  if (!dependent_field_) {
    return;
  }
  std::map<string, string> variables(variables_);
  // For the CRTP base class, all mutation methods are dependent, and so
  // they must be in the header.
  variables["dependent_classname"] =
      DependentBaseClassTemplateName(descriptor_->containing_type()) + "<T>";
  variables["this_message"] = DependentBaseDownCast();
  variables["this_const_message"] = DependentBaseConstDownCast();

  if (dependent_getter_) {
    printer->Print(variables,
      "template <class T>\n"
      "inline const $type$& $dependent_classname$::$name$(int index) const {\n"
      "  // @@protoc_insertion_point(field_get:$full_name$)\n"
      "  return $this_const_message$$name$_.$cppget$(index);\n"
      "}\n");
  }

  // Generate per-element accessors:
  printer->Print(variables,
    "template <class T>\n"
    "inline $type$* $dependent_classname$::mutable_$name$(int index) {\n"
    // TODO(dlj): move insertion points
    "  // @@protoc_insertion_point(field_mutable:$full_name$)\n"
    "  return $this_message$$name$_.Mutable(index);\n"
    "}\n"
    "template <class T>\n"
    "inline $type$* $dependent_classname$::add_$name$() {\n"
    "  // @@protoc_insertion_point(field_add:$full_name$)\n"
    "  return $this_message$$name$_.Add();\n"
    "}\n");

  if (dependent_getter_) {
    printer->Print(variables,
      "template <class T>\n"
      "inline const ::google::protobuf::RepeatedPtrField< $type$ >&\n"
      "$dependent_classname$::$name$() const {\n"
      "  // @@protoc_insertion_point(field_list:$full_name$)\n"
      "  return $this_const_message$$name$_;\n"
      "}\n");
  }

  // Generate mutable access to the entire list:
  printer->Print(variables,
    "template <class T>\n"
    "inline ::google::protobuf::RepeatedPtrField< $type$ >*\n"
    "$dependent_classname$::mutable_$name$() {\n"
    "  // @@protoc_insertion_point(field_mutable_list:$full_name$)\n"
    "  return &$this_message$$name$_;\n"
    "}\n");
}

void RepeatedMessageFieldGenerator::
GenerateInlineAccessorDefinitions(io::Printer* printer) const {
  if (!dependent_getter_) {
    printer->Print(variables_,
      "inline const $type$& $classname$::$name$(int index) const {\n"
      "  // @@protoc_insertion_point(field_get:$full_name$)\n"
      "  return $name$_.$cppget$(index);\n"
      "}\n");
  }

  if (!dependent_field_) {
    printer->Print(variables_,
      "inline $type$* $classname$::mutable_$name$(int index) {\n"
      // TODO(dlj): move insertion points
      "  // @@protoc_insertion_point(field_mutable:$full_name$)\n"
      "  return $name$_.Mutable(index);\n"
      "}\n"
      "inline $type$* $classname$::add_$name$() {\n"
      "  // @@protoc_insertion_point(field_add:$full_name$)\n"
      "  return $name$_.Add();\n"
      "}\n");
  }

  if (!dependent_field_) {
    printer->Print(variables_,
      "inline ::google::protobuf::RepeatedPtrField< $type$ >*\n"
      "$classname$::mutable_$name$() {\n"
      "  // @@protoc_insertion_point(field_mutable_list:$full_name$)\n"
      "  return &$name$_;\n"
      "}\n");
  }
  if (!dependent_getter_) {
    printer->Print(variables_,
      "inline const ::google::protobuf::RepeatedPtrField< $type$ >&\n"
      "$classname$::$name$() const {\n"
      "  // @@protoc_insertion_point(field_list:$full_name$)\n"
      "  return $name$_;\n"
      "}\n");
  }
}

void RepeatedMessageFieldGenerator::
GenerateClearingCode(io::Printer* printer) const {
  printer->Print(variables_, "$name$_.Clear();\n");
}

void RepeatedMessageFieldGenerator::
GenerateMergingCode(io::Printer* printer) const {
  printer->Print(variables_, "$name$_.MergeFrom(from.$name$_);\n");
}

void RepeatedMessageFieldGenerator::
GenerateSwappingCode(io::Printer* printer) const {
  printer->Print(variables_, "$name$_.InternalSwap(&other->$name$_);\n");
}

void RepeatedMessageFieldGenerator::
GenerateConstructorCode(io::Printer* printer) const {
  // Not needed for repeated fields.
}

void RepeatedMessageFieldGenerator::
GenerateMergeFromCodedStream(io::Printer* printer) const {
  if (descriptor_->type() == FieldDescriptor::TYPE_MESSAGE) {
    printer->Print(variables_,
      "DO_(::google::protobuf::internal::WireFormatLite::"
      "ReadMessage(input, add_$name$()));\n");
  } else {
    printer->Print(variables_,
      "DO_(::google::protobuf::internal::WireFormatLite::"
      "ReadGroup($number$, input, add_$name$()));\n");
  }
}

void RepeatedMessageFieldGenerator::
GenerateSerializeWithCachedSizes(io::Printer* printer) const {
  printer->Print(variables_,
    "for (unsigned int i = 0,\n"
    "    n = static_cast<unsigned int>(this->$name$_size()); i < n; i++) {\n"
    "  ::google::protobuf::internal::WireFormatLite::Write$stream_writer$(\n"
    "    $number$, this->$name$(static_cast<int>(i)), output);\n"
    "}\n");
}

void RepeatedMessageFieldGenerator::
GenerateSerializeWithCachedSizesToArray(io::Printer* printer) const {
  printer->Print(variables_,
    "for (unsigned int i = 0,\n"
    "    n = static_cast<unsigned int>(this->$name$_size()); i < n; i++) {\n"
    "  target = ::google::protobuf::internal::WireFormatLite::\n"
    "    InternalWrite$declared_type$ToArray(\n"
    "      $number$, this->$name$(static_cast<int>(i)), deterministic, target);\n"
    "}\n");
}

void RepeatedMessageFieldGenerator::
GenerateByteSize(io::Printer* printer) const {
  printer->Print(variables_,
    "{\n"
    "  unsigned int count = static_cast<unsigned int>(this->$name$_size());\n");
  printer->Indent();
  printer->Print(variables_,
    "total_size += $tag_size$UL * count;\n"
    "for (unsigned int i = 0; i < count; i++) {\n"
    "  total_size +=\n"
    "    ::google::protobuf::internal::WireFormatLite::$declared_type$Size(\n"
    "      this->$name$(static_cast<int>(i)));\n"
    "}\n");
  printer->Outdent();
  printer->Print("}\n");
}

}  // namespace cpp
}  // namespace compiler
}  // namespace protobuf
}  // namespace google
