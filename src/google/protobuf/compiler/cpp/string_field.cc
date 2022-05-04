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

#include <google/protobuf/compiler/cpp/string_field.h>

#include <google/protobuf/io/printer.h>
#include <google/protobuf/stubs/strutil.h>
#include <google/protobuf/compiler/cpp/helpers.h>
#include <google/protobuf/descriptor.pb.h>


namespace google {
namespace protobuf {
namespace compiler {
namespace cpp {

namespace {

void SetStringVariables(const FieldDescriptor* descriptor,
                        std::map<std::string, std::string>* variables,
                        const Options& options) {
  SetCommonFieldVariables(descriptor, variables, options);

  const std::string kNS = "::" + (*variables)["proto_ns"] + "::internal::";
  const std::string kArenaStringPtr = kNS + "ArenaStringPtr";

  (*variables)["default"] = DefaultValue(options, descriptor);
  (*variables)["default_length"] =
      StrCat(descriptor->default_value_string().length());
  (*variables)["default_variable_name"] = MakeDefaultName(descriptor);
  (*variables)["default_variable_field"] = MakeDefaultFieldName(descriptor);

  if (descriptor->default_value_string().empty()) {
    (*variables)["default_string"] = kNS + "GetEmptyStringAlreadyInited()";
    (*variables)["default_value"] = "&" + (*variables)["default_string"];
    (*variables)["lazy_variable_args"] = "";
  } else {
    (*variables)["lazy_variable"] =
        StrCat(QualifiedClassName(descriptor->containing_type(), options),
                     "::", MakeDefaultFieldName(descriptor));

    (*variables)["default_string"] = (*variables)["lazy_variable"] + ".get()";
    (*variables)["default_value"] = "nullptr";
    (*variables)["lazy_variable_args"] = (*variables)["lazy_variable"] + ", ";
  }

  (*variables)["pointer_type"] =
      descriptor->type() == FieldDescriptor::TYPE_BYTES ? "void" : "char";
  (*variables)["setter"] =
      descriptor->type() == FieldDescriptor::TYPE_BYTES ? "SetBytes" : "Set";
  (*variables)["null_check"] = (*variables)["DCHK"] + "(value != nullptr);\n";
  // NOTE: Escaped here to unblock proto1->proto2 migration.
  // TODO(liujisi): Extend this to apply for other conflicting methods.
  (*variables)["release_name"] =
      SafeFunctionName(descriptor->containing_type(), descriptor, "release_");
  (*variables)["full_name"] = descriptor->full_name();

  if (options.opensource_runtime) {
    (*variables)["string_piece"] = "::std::string";
  } else {
    (*variables)["string_piece"] = "::StringPiece";
  }
}

}  // namespace

// ===================================================================

StringFieldGenerator::StringFieldGenerator(const FieldDescriptor* descriptor,
                                           const Options& options)
    : FieldGenerator(descriptor, options),
      inlined_(IsStringInlined(descriptor, options)) {
  SetStringVariables(descriptor, &variables_, options);
}

StringFieldGenerator::~StringFieldGenerator() {}

void StringFieldGenerator::GeneratePrivateMembers(io::Printer* printer) const {
  Formatter format(printer, variables_);
  if (!inlined_) {
    format("::$proto_ns$::internal::ArenaStringPtr $name$_;\n");
  } else {
    // Skips the automatic destruction; rather calls it explicitly if
    // allocating arena is null. This is required to support message-owned
    // arena (go/path-to-arenas) where a root proto is destroyed but
    // InlinedStringField may have arena-allocated memory.
    format("::$proto_ns$::internal::InlinedStringField $name$_;\n");
  }
}

void StringFieldGenerator::GenerateStaticMembers(io::Printer* printer) const {
  Formatter format(printer, variables_);
  if (!descriptor_->default_value_string().empty()) {
    format(
        "static const ::$proto_ns$::internal::LazyString"
        " $default_variable_name$;\n");
  }
  if (inlined_) {
    // `_init_inline_xxx` is used for initializing default instances.
    format("static std::true_type _init_inline_$name$_;\n");
  }
}

void StringFieldGenerator::GenerateAccessorDeclarations(
    io::Printer* printer) const {
  Formatter format(printer, variables_);
  // If we're using StringFieldGenerator for a field with a ctype, it's
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

  bool unknown_ctype = descriptor_->options().ctype() !=
                       EffectiveStringCType(descriptor_, options_);

  if (unknown_ctype) {
    format.Outdent();
    format(
        " private:\n"
        "  // Hidden due to unknown ctype option.\n");
    format.Indent();
  }

  format(
      "$deprecated_attr$const std::string& ${1$$name$$}$() const;\n"
      "template <typename ArgT0 = const std::string&, typename... ArgT>\n"
      "$deprecated_attr$void ${1$set_$name$$}$(ArgT0&& arg0, ArgT... args);\n",
      descriptor_);
  format(
      "$deprecated_attr$std::string* ${1$mutable_$name$$}$();\n"
      "PROTOBUF_NODISCARD $deprecated_attr$std::string* "
      "${1$$release_name$$}$();\n"
      "$deprecated_attr$void ${1$set_allocated_$name$$}$(std::string* "
      "$name$);\n",
      descriptor_);
  format(
      "private:\n"
      "const std::string& _internal_$name$() const;\n"
      "inline PROTOBUF_ALWAYS_INLINE void "
      "_internal_set_$name$(const std::string& value);\n"
      "std::string* _internal_mutable_$name$();\n");
  if (inlined_) {
    format(
        "inline PROTOBUF_ALWAYS_INLINE bool _internal_$name$_donated() "
        "const;\n");
  }
  format("public:\n");

  if (unknown_ctype) {
    format.Outdent();
    format(" public:\n");
    format.Indent();
  }
}

void StringFieldGenerator::GenerateInlineAccessorDefinitions(
    io::Printer* printer) const {
  Formatter format(printer, variables_);
  format(
      "inline const std::string& $classname$::$name$() const {\n"
      "$annotate_get$"
      "  // @@protoc_insertion_point(field_get:$full_name$)\n");
  if (!descriptor_->default_value_string().empty()) {
    format(
        "  if ($field$.IsDefault()) return "
        "$default_variable_field$.get();\n");
  }
  format(
      "  return _internal_$name$();\n"
      "}\n");
  if (!inlined_) {
    format(
        "template <typename ArgT0, typename... ArgT>\n"
        "inline PROTOBUF_ALWAYS_INLINE\n"
        "void $classname$::set_$name$(ArgT0&& arg0, ArgT... args) {\n"
        "$maybe_prepare_split_message$"
        " $set_hasbit$\n"
        " $field$.$setter$(static_cast<ArgT0 &&>(arg0),"
        " args..., GetArenaForAllocation());\n"
        "$annotate_set$"
        "  // @@protoc_insertion_point(field_set:$full_name$)\n"
        "}\n");
  } else {
    format(
        "template <typename ArgT0, typename... ArgT>\n"
        "inline PROTOBUF_ALWAYS_INLINE\n"
        "void $classname$::set_$name$(ArgT0&& arg0, ArgT... args) {\n"
        "$maybe_prepare_split_message$"
        " $set_hasbit$\n"
        " $field$.$setter$(static_cast<ArgT0 &&>(arg0),"
        " args..., GetArenaForAllocation(), _internal_$name$_donated(), "
        "&$donating_states_word$, $mask_for_undonate$, this);\n"
        "$annotate_set$"
        "  // @@protoc_insertion_point(field_set:$full_name$)\n"
        "}\n"
        "inline bool $classname$::_internal_$name$_donated() const {\n"
        "  bool value = $inlined_string_donated$\n"
        "  return value;\n"
        "}\n");
  }
  format(
      "inline std::string* $classname$::mutable_$name$() {\n"
      "$maybe_prepare_split_message$"
      "  std::string* _s = _internal_mutable_$name$();\n"
      "$annotate_mutable$"
      "  // @@protoc_insertion_point(field_mutable:$full_name$)\n"
      "  return _s;\n"
      "}\n"
      "inline const std::string& $classname$::_internal_$name$() const {\n"
      "  return $field$.Get();\n"
      "}\n"
      "inline void $classname$::_internal_set_$name$(const std::string& "
      "value) {\n"
      "  $set_hasbit$\n");
  if (!inlined_) {
    format(
        "  $field$.Set(value, GetArenaForAllocation());\n"
        "}\n");
  } else {
    format(
        "  $field$.Set(value, GetArenaForAllocation(),\n"
        "    _internal_$name$_donated(), &$donating_states_word$, "
        "$mask_for_undonate$, this);\n"
        "}\n");
  }
  format(
      "inline std::string* $classname$::_internal_mutable_$name$() {\n"
      "  $set_hasbit$\n");
  if (!inlined_) {
    format(
        "  return $field$.Mutable($lazy_variable_args$"
        "GetArenaForAllocation());\n"
        "}\n");
  } else {
    format(
        "  return $field$.Mutable($lazy_variable_args$"
        "GetArenaForAllocation(), _internal_$name$_donated(), "
        "&$donating_states_word$, $mask_for_undonate$, this);\n"
        "}\n");
  }
  format(
      "inline std::string* $classname$::$release_name$() {\n"
      "$annotate_release$"
      "$maybe_prepare_split_message$"
      "  // @@protoc_insertion_point(field_release:$full_name$)\n");

  if (HasHasbit(descriptor_)) {
    format(
        "  if (!_internal_has_$name$()) {\n"
        "    return nullptr;\n"
        "  }\n"
        "  $clear_hasbit$\n");
    if (!inlined_) {
      format("  auto* p = $field$.Release();\n");
      if (descriptor_->default_value_string().empty()) {
        format(
            "#ifdef PROTOBUF_FORCE_COPY_DEFAULT_STRING\n"
            "  if ($field$.IsDefault()) {\n"
            "    $field$.Set(\"\", GetArenaForAllocation());\n"
            "  }\n"
            "#endif // PROTOBUF_FORCE_COPY_DEFAULT_STRING\n");
      }
      format("  return p;\n");
    } else {
      format(
          "  return $field$.Release(GetArenaForAllocation(), "
          "_internal_$name$_donated());\n");
    }
  } else {
    format("  return $field$.Release();\n");
  }

  format(
      "}\n"
      "inline void $classname$::set_allocated_$name$(std::string* $name$) {\n"
      "$maybe_prepare_split_message$"
      "  if ($name$ != nullptr) {\n"
      "    $set_hasbit$\n"
      "  } else {\n"
      "    $clear_hasbit$\n"
      "  }\n");
  if (!inlined_) {
    format("  $field$.SetAllocated($name$, GetArenaForAllocation());\n");
    if (descriptor_->default_value_string().empty()) {
      format(
          "#ifdef PROTOBUF_FORCE_COPY_DEFAULT_STRING\n"
          "  if ($field$.IsDefault()) {\n"
          "    $field$.Set(\"\", GetArenaForAllocation());\n"
          "  }\n"
          "#endif // PROTOBUF_FORCE_COPY_DEFAULT_STRING\n");
    }
  } else {
    // Currently, string fields with default value can't be inlined.
    format(
        "    $field$.SetAllocated(nullptr, $name$, GetArenaForAllocation(), "
        "_internal_$name$_donated(), &$donating_states_word$, "
        "$mask_for_undonate$, this);\n");
  }
  format(
      "$annotate_set$"
      "  // @@protoc_insertion_point(field_set_allocated:$full_name$)\n"
      "}\n");
}

void StringFieldGenerator::GenerateNonInlineAccessorDefinitions(
    io::Printer* printer) const {
  Formatter format(printer, variables_);
  if (!descriptor_->default_value_string().empty()) {
    format(
        "const ::$proto_ns$::internal::LazyString "
        "$classname$::$default_variable_field$"
        "{{{$default$, $default_length$}}, {nullptr}};\n");
  }
}

void StringFieldGenerator::GenerateClearingCode(io::Printer* printer) const {
  Formatter format(printer, variables_);
  if (descriptor_->default_value_string().empty()) {
    format("$field$.ClearToEmpty();\n");
  } else {
    GOOGLE_DCHECK(!inlined_);
    format(
        "$field$.ClearToDefault($lazy_variable$, GetArenaForAllocation());\n");
  }
}

void StringFieldGenerator::GenerateMessageClearingCode(
    io::Printer* printer) const {
  Formatter format(printer, variables_);
  // Two-dimension specialization here: supporting arenas, field presence, or
  // not, and default value is the empty string or not. Complexity here ensures
  // the minimal number of branches / amount of extraneous code at runtime
  // (given that the below methods are inlined one-liners)!

  // If we have a hasbit, then the Clear() method of the protocol buffer
  // will have checked that this field is set.  If so, we can avoid redundant
  // checks against the default variable.
  const bool must_be_present = HasHasbit(descriptor_);

  if (inlined_ && must_be_present) {
    // Calling mutable_$name$() gives us a string reference and sets the has bit
    // for $name$ (in proto2).  We may get here when the string field is inlined
    // but the string's contents have not been changed by the user, so we cannot
    // make an assertion about the contents of the string and could never make
    // an assertion about the string instance.
    //
    // For non-inlined strings, we distinguish from non-default by comparing
    // instances, rather than contents.
    format("$DCHK$(!$field$.IsDefault());\n");
  }

  if (descriptor_->default_value_string().empty()) {
    if (must_be_present) {
      format("$field$.ClearNonDefaultToEmpty();\n");
    } else {
      format("$field$.ClearToEmpty();\n");
    }
  } else {
    // Clear to a non-empty default is more involved, as we try to use the
    // Arena if one is present and may need to reallocate the string.
    format(
        "$field$.ClearToDefault($lazy_variable$, GetArenaForAllocation());\n ");
  }
}

void StringFieldGenerator::GenerateMergingCode(io::Printer* printer) const {
  Formatter format(printer, variables_);
  // TODO(gpike): improve this
  format("_this->_internal_set_$name$(from._internal_$name$());\n");
}

void StringFieldGenerator::GenerateSwappingCode(io::Printer* printer) const {
  Formatter format(printer, variables_);
  if (!inlined_) {
    format(
        "::$proto_ns$::internal::ArenaStringPtr::InternalSwap(\n"
        "    &$field$, lhs_arena,\n"
        "    &other->$field$, rhs_arena\n"
        ");\n");
  } else {
    format(
        "::$proto_ns$::internal::InlinedStringField::InternalSwap(\n"
        "  &$field$, lhs_arena, "
        "($inlined_string_donated_array$[0] & 0x1u) == 0, this,\n"
        "  &other->$field$, rhs_arena, "
        "(other->$inlined_string_donated_array$[0] & 0x1u) == 0, other);\n");
  }
}

void StringFieldGenerator::GenerateConstructorCode(io::Printer* printer) const {
  Formatter format(printer, variables_);
  if (inlined_ && descriptor_->default_value_string().empty()) {
    return;
  }
  GOOGLE_DCHECK(!inlined_);
  format("$field$.InitDefault();\n");
  if (IsString(descriptor_, options_) &&
      descriptor_->default_value_string().empty()) {
    format(
        "#ifdef PROTOBUF_FORCE_COPY_DEFAULT_STRING\n"
        "  $field$.Set(\"\", GetArenaForAllocation());\n"
        "#endif // PROTOBUF_FORCE_COPY_DEFAULT_STRING\n");
  }
}

void StringFieldGenerator::GenerateCreateSplitMessageCode(
    io::Printer* printer) const {
  GOOGLE_CHECK(ShouldSplit(descriptor_, options_));
  GOOGLE_CHECK(!inlined_);
  Formatter format(printer, variables_);
  format("ptr->$name$_.InitDefault();\n");
  if (IsString(descriptor_, options_) &&
      descriptor_->default_value_string().empty()) {
    format(
        "#ifdef PROTOBUF_FORCE_COPY_DEFAULT_STRING\n"
        "  ptr->$name$_.Set(\"\", GetArenaForAllocation());\n"
        "#endif // PROTOBUF_FORCE_COPY_DEFAULT_STRING\n");
  }
}

void StringFieldGenerator::GenerateCopyConstructorCode(
    io::Printer* printer) const {
  Formatter format(printer, variables_);
  GenerateConstructorCode(printer);
  if (inlined_) {
    format("new (&_this->$field$) ::_pbi::InlinedStringField();\n");
  }

  if (HasHasbit(descriptor_)) {
    format("if (from._internal_has_$name$()) {\n");
  } else {
    format("if (!from._internal_$name$().empty()) {\n");
  }

  format.Indent();

  if (!inlined_) {
    format(
        "_this->$field$.Set(from._internal_$name$(), \n"
        "  _this->GetArenaForAllocation());\n");
  } else {
    format(
        "_this->$field$.Set(from._internal_$name$(),\n"
        "  _this->GetArenaForAllocation(), _this->_internal_$name$_donated(), "
        "&_this->$donating_states_word$, $mask_for_undonate$, _this);\n");
  }

  format.Outdent();
  format("}\n");
}

void StringFieldGenerator::GenerateDestructorCode(io::Printer* printer) const {
  Formatter format(printer, variables_);
  if (!inlined_) {
    if (ShouldSplit(descriptor_, options_)) {
      format("$cached_split_ptr$->$name$_.Destroy();\n");
      return;
    }
    format("$field$.Destroy();\n");
    return;
  }
  // Explicitly calls ~InlinedStringField as its automatic call is disabled.
  // Destructor has been implicitly skipped as a union, and even the
  // message-owned arena is enabled, arena could still be missing for
  // Arena::CreateMessage(nullptr).
  GOOGLE_DCHECK(!ShouldSplit(descriptor_, options_));
  format("$field$.~InlinedStringField();\n");
}

ArenaDtorNeeds StringFieldGenerator::NeedsArenaDestructor() const {
  return inlined_ ? ArenaDtorNeeds::kOnDemand : ArenaDtorNeeds::kNone;
}

void StringFieldGenerator::GenerateArenaDestructorCode(
    io::Printer* printer) const {
  if (!inlined_) return;
  Formatter format(printer, variables_);
  // _this is the object being destructed (we are inside a static method here).
  format(
      "if (!_this->_internal_$name$_donated()) {\n"
      "  _this->$field$.~InlinedStringField();\n"
      "}\n");
}

void StringFieldGenerator::GenerateSerializeWithCachedSizesToArray(
    io::Printer* printer) const {
  Formatter format(printer, variables_);
  if (descriptor_->type() == FieldDescriptor::TYPE_STRING) {
    GenerateUtf8CheckCodeForString(
        descriptor_, options_, false,
        "this->_internal_$name$().data(), "
        "static_cast<int>(this->_internal_$name$().length()),\n",
        format);
  }
  format(
      "target = stream->Write$declared_type$MaybeAliased(\n"
      "    $number$, this->_internal_$name$(), target);\n");
}

void StringFieldGenerator::GenerateByteSize(io::Printer* printer) const {
  Formatter format(printer, variables_);
  format(
      "total_size += $tag_size$ +\n"
      "  ::$proto_ns$::internal::WireFormatLite::$declared_type$Size(\n"
      "    this->_internal_$name$());\n");
}

void StringFieldGenerator::GenerateConstexprAggregateInitializer(
    io::Printer* printer) const {
  Formatter format(printer, variables_);
  if (inlined_) {
    format("/*decltype($field$)*/{nullptr, false}");
    return;
  }
  if (descriptor_->default_value_string().empty()) {
    format(
        "/*decltype($field$)*/{&::_pbi::fixed_address_empty_string, "
        "::_pbi::ConstantInitialized{}}");
  } else {
    format("/*decltype($field$)*/{nullptr, ::_pbi::ConstantInitialized{}}");
  }
}

void StringFieldGenerator::GenerateAggregateInitializer(
    io::Printer* printer) const {
  Formatter format(printer, variables_);
  if (ShouldSplit(descriptor_, options_)) {
    GOOGLE_CHECK(!inlined_);
    format("decltype(Impl_::Split::$name$_){}");
    return;
  }
  if (!inlined_) {
    format("decltype($field$){}");
  } else {
    format("decltype($field$)(arena)");
  }
}

void StringFieldGenerator::GenerateCopyAggregateInitializer(
    io::Printer* printer) const {
  Formatter format(printer, variables_);
  format("decltype($field$){}");
}

// ===================================================================

StringOneofFieldGenerator::StringOneofFieldGenerator(
    const FieldDescriptor* descriptor, const Options& options)
    : StringFieldGenerator(descriptor, options) {
  SetCommonOneofFieldVariables(descriptor, &variables_);
  variables_["field_name"] = UnderscoresToCamelCase(descriptor->name(), true);
  variables_["oneof_index"] =
      StrCat(descriptor->containing_oneof()->index());
}

StringOneofFieldGenerator::~StringOneofFieldGenerator() {}

void StringOneofFieldGenerator::GenerateInlineAccessorDefinitions(
    io::Printer* printer) const {
  Formatter format(printer, variables_);
  format(
      "inline const std::string& $classname$::$name$() const {\n"
      "$annotate_get$"
      "  // @@protoc_insertion_point(field_get:$full_name$)\n"
      "  return _internal_$name$();\n"
      "}\n"
      "template <typename ArgT0, typename... ArgT>\n"
      "inline void $classname$::set_$name$(ArgT0&& arg0, ArgT... args) {\n"
      "  if (!_internal_has_$name$()) {\n"
      "    clear_$oneof_name$();\n"
      "    set_has_$name$();\n"
      "    $field$.InitDefault();\n"
      "  }\n"
      "  $field$.$setter$("
      " static_cast<ArgT0 &&>(arg0), args..., GetArenaForAllocation());\n"
      "$annotate_set$"
      "  // @@protoc_insertion_point(field_set:$full_name$)\n"
      "}\n"
      "inline std::string* $classname$::mutable_$name$() {\n"
      "  std::string* _s = _internal_mutable_$name$();\n"
      "$annotate_mutable$"
      "  // @@protoc_insertion_point(field_mutable:$full_name$)\n"
      "  return _s;\n"
      "}\n"
      "inline const std::string& $classname$::_internal_$name$() const {\n"
      "  if (_internal_has_$name$()) {\n"
      "    return $field$.Get();\n"
      "  }\n"
      "  return $default_string$;\n"
      "}\n"
      "inline void $classname$::_internal_set_$name$(const std::string& "
      "value) {\n"
      "  if (!_internal_has_$name$()) {\n"
      "    clear_$oneof_name$();\n"
      "    set_has_$name$();\n"
      "    $field$.InitDefault();\n"
      "  }\n"
      "  $field$.Set(value, GetArenaForAllocation());\n"
      "}\n");
  format(
      "inline std::string* $classname$::_internal_mutable_$name$() {\n"
      "  if (!_internal_has_$name$()) {\n"
      "    clear_$oneof_name$();\n"
      "    set_has_$name$();\n"
      "    $field$.InitDefault();\n"
      "  }\n"
      "  return $field$.Mutable($lazy_variable_args$"
      "      GetArenaForAllocation());\n"
      "}\n"
      "inline std::string* $classname$::$release_name$() {\n"
      "$annotate_release$"
      "  // @@protoc_insertion_point(field_release:$full_name$)\n"
      "  if (_internal_has_$name$()) {\n"
      "    clear_has_$oneof_name$();\n"
      "    return $field$.Release();\n"
      "  } else {\n"
      "    return nullptr;\n"
      "  }\n"
      "}\n"
      "inline void $classname$::set_allocated_$name$(std::string* $name$) {\n"
      "  if (has_$oneof_name$()) {\n"
      "    clear_$oneof_name$();\n"
      "  }\n"
      "  if ($name$ != nullptr) {\n"
      "    set_has_$name$();\n"
      "    $field$.InitAllocated($name$, GetArenaForAllocation());\n"
      "  }\n"
      "$annotate_set$"
      "  // @@protoc_insertion_point(field_set_allocated:$full_name$)\n"
      "}\n");
}

void StringOneofFieldGenerator::GenerateClearingCode(
    io::Printer* printer) const {
  Formatter format(printer, variables_);
  format("$field$.Destroy();\n");
}

void StringOneofFieldGenerator::GenerateMessageClearingCode(
    io::Printer* printer) const {
  return GenerateClearingCode(printer);
}

void StringOneofFieldGenerator::GenerateSwappingCode(
    io::Printer* printer) const {
  // Don't print any swapping code. Swapping the union will swap this field.
}

void StringOneofFieldGenerator::GenerateConstructorCode(
    io::Printer* printer) const {
  // Nothing required here.
}

// ===================================================================

RepeatedStringFieldGenerator::RepeatedStringFieldGenerator(
    const FieldDescriptor* descriptor, const Options& options)
    : FieldGenerator(descriptor, options) {
  SetStringVariables(descriptor, &variables_, options);
}

RepeatedStringFieldGenerator::~RepeatedStringFieldGenerator() {}

void RepeatedStringFieldGenerator::GeneratePrivateMembers(
    io::Printer* printer) const {
  Formatter format(printer, variables_);
  format("::$proto_ns$::RepeatedPtrField<std::string> $name$_;\n");
}

void RepeatedStringFieldGenerator::GenerateAccessorDeclarations(
    io::Printer* printer) const {
  Formatter format(printer, variables_);
  // See comment above about unknown ctypes.
  bool unknown_ctype = descriptor_->options().ctype() !=
                       EffectiveStringCType(descriptor_, options_);

  if (unknown_ctype) {
    format.Outdent();
    format(
        " private:\n"
        "  // Hidden due to unknown ctype option.\n");
    format.Indent();
  }

  format(
      "$deprecated_attr$const std::string& ${1$$name$$}$(int index) const;\n"
      "$deprecated_attr$std::string* ${1$mutable_$name$$}$(int index);\n"
      "$deprecated_attr$void ${1$set_$name$$}$(int index, const "
      "std::string& value);\n"
      "$deprecated_attr$void ${1$set_$name$$}$(int index, std::string&& "
      "value);\n"
      "$deprecated_attr$void ${1$set_$name$$}$(int index, const "
      "char* value);\n",
      descriptor_);
  if (!options_.opensource_runtime) {
    format(
        "$deprecated_attr$void ${1$set_$name$$}$(int index, "
        "StringPiece value);\n",
        descriptor_);
  }
  format(
      "$deprecated_attr$void ${1$set_$name$$}$("
      "int index, const $pointer_type$* value, size_t size);\n"
      "$deprecated_attr$std::string* ${1$add_$name$$}$();\n"
      "$deprecated_attr$void ${1$add_$name$$}$(const std::string& value);\n"
      "$deprecated_attr$void ${1$add_$name$$}$(std::string&& value);\n"
      "$deprecated_attr$void ${1$add_$name$$}$(const char* value);\n",
      descriptor_);
  if (!options_.opensource_runtime) {
    format(
        "$deprecated_attr$void ${1$add_$name$$}$(StringPiece value);\n",
        descriptor_);
  }
  format(
      "$deprecated_attr$void ${1$add_$name$$}$(const $pointer_type$* "
      "value, size_t size)"
      ";\n"
      "$deprecated_attr$const ::$proto_ns$::RepeatedPtrField<std::string>& "
      "${1$$name$$}$() "
      "const;\n"
      "$deprecated_attr$::$proto_ns$::RepeatedPtrField<std::string>* "
      "${1$mutable_$name$$}$()"
      ";\n"
      "private:\n"
      "const std::string& ${1$_internal_$name$$}$(int index) const;\n"
      "std::string* _internal_add_$name$();\n"
      "public:\n",
      descriptor_);

  if (unknown_ctype) {
    format.Outdent();
    format(" public:\n");
    format.Indent();
  }
}

void RepeatedStringFieldGenerator::GenerateInlineAccessorDefinitions(
    io::Printer* printer) const {
  Formatter format(printer, variables_);
  format(
      "inline std::string* $classname$::add_$name$() {\n"
      "  std::string* _s = _internal_add_$name$();\n"
      "$annotate_add_mutable$"
      "  // @@protoc_insertion_point(field_add_mutable:$full_name$)\n"
      "  return _s;\n"
      "}\n");
  if (options_.safe_boundary_check) {
    format(
        "inline const std::string& $classname$::_internal_$name$(int index) "
        "const {\n"
        "  return $field$.InternalCheckedGet(\n"
        "      index, ::$proto_ns$::internal::GetEmptyStringAlreadyInited());\n"
        "}\n");
  } else {
    format(
        "inline const std::string& $classname$::_internal_$name$(int index) "
        "const {\n"
        "  return $field$.Get(index);\n"
        "}\n");
  }
  format(
      "inline const std::string& $classname$::$name$(int index) const {\n"
      "$annotate_get$"
      "  // @@protoc_insertion_point(field_get:$full_name$)\n"
      "  return _internal_$name$(index);\n"
      "}\n"
      "inline std::string* $classname$::mutable_$name$(int index) {\n"
      "$annotate_mutable$"
      "  // @@protoc_insertion_point(field_mutable:$full_name$)\n"
      "  return $field$.Mutable(index);\n"
      "}\n"
      "inline void $classname$::set_$name$(int index, const std::string& "
      "value) "
      "{\n"
      "  $field$.Mutable(index)->assign(value);\n"
      "$annotate_set$"
      "  // @@protoc_insertion_point(field_set:$full_name$)\n"
      "}\n"
      "inline void $classname$::set_$name$(int index, std::string&& value) {\n"
      "  $field$.Mutable(index)->assign(std::move(value));\n"
      "$annotate_set$"
      "  // @@protoc_insertion_point(field_set:$full_name$)\n"
      "}\n"
      "inline void $classname$::set_$name$(int index, const char* value) {\n"
      "  $null_check$"
      "  $field$.Mutable(index)->assign(value);\n"
      "$annotate_set$"
      "  // @@protoc_insertion_point(field_set_char:$full_name$)\n"
      "}\n");
  if (!options_.opensource_runtime) {
    format(
        "inline void "
        "$classname$::set_$name$(int index, StringPiece value) {\n"
        "  $field$.Mutable(index)->assign(value.data(), value.size());\n"
        "$annotate_set$"
        "  // @@protoc_insertion_point(field_set_string_piece:$full_name$)\n"
        "}\n");
  }
  format(
      "inline void "
      "$classname$::set_$name$"
      "(int index, const $pointer_type$* value, size_t size) {\n"
      "  $field$.Mutable(index)->assign(\n"
      "    reinterpret_cast<const char*>(value), size);\n"
      "$annotate_set$"
      "  // @@protoc_insertion_point(field_set_pointer:$full_name$)\n"
      "}\n"
      "inline std::string* $classname$::_internal_add_$name$() {\n"
      "  return $field$.Add();\n"
      "}\n"
      "inline void $classname$::add_$name$(const std::string& value) {\n"
      "  $field$.Add()->assign(value);\n"
      "$annotate_add$"
      "  // @@protoc_insertion_point(field_add:$full_name$)\n"
      "}\n"
      "inline void $classname$::add_$name$(std::string&& value) {\n"
      "  $field$.Add(std::move(value));\n"
      "$annotate_add$"
      "  // @@protoc_insertion_point(field_add:$full_name$)\n"
      "}\n"
      "inline void $classname$::add_$name$(const char* value) {\n"
      "  $null_check$"
      "  $field$.Add()->assign(value);\n"
      "$annotate_add$"
      "  // @@protoc_insertion_point(field_add_char:$full_name$)\n"
      "}\n");
  if (!options_.opensource_runtime) {
    format(
        "inline void $classname$::add_$name$(StringPiece value) {\n"
        "  $field$.Add()->assign(value.data(), value.size());\n"
        "$annotate_add$"
        "  // @@protoc_insertion_point(field_add_string_piece:$full_name$)\n"
        "}\n");
  }
  format(
      "inline void "
      "$classname$::add_$name$(const $pointer_type$* value, size_t size) {\n"
      "  $field$.Add()->assign(reinterpret_cast<const char*>(value), size);\n"
      "$annotate_add$"
      "  // @@protoc_insertion_point(field_add_pointer:$full_name$)\n"
      "}\n"
      "inline const ::$proto_ns$::RepeatedPtrField<std::string>&\n"
      "$classname$::$name$() const {\n"
      "$annotate_list$"
      "  // @@protoc_insertion_point(field_list:$full_name$)\n"
      "  return $field$;\n"
      "}\n"
      "inline ::$proto_ns$::RepeatedPtrField<std::string>*\n"
      "$classname$::mutable_$name$() {\n"
      "$annotate_mutable_list$"
      "  // @@protoc_insertion_point(field_mutable_list:$full_name$)\n"
      "  return &$field$;\n"
      "}\n");
}

void RepeatedStringFieldGenerator::GenerateClearingCode(
    io::Printer* printer) const {
  Formatter format(printer, variables_);
  format("$field$.Clear();\n");
}

void RepeatedStringFieldGenerator::GenerateMergingCode(
    io::Printer* printer) const {
  Formatter format(printer, variables_);
  format("_this->$field$.MergeFrom(from.$field$);\n");
}

void RepeatedStringFieldGenerator::GenerateSwappingCode(
    io::Printer* printer) const {
  Formatter format(printer, variables_);
  format("$field$.InternalSwap(&other->$field$);\n");
}

void RepeatedStringFieldGenerator::GenerateDestructorCode(
    io::Printer* printer) const {
  Formatter format(printer, variables_);
  format("$field$.~RepeatedPtrField();\n");
}

void RepeatedStringFieldGenerator::GenerateSerializeWithCachedSizesToArray(
    io::Printer* printer) const {
  Formatter format(printer, variables_);
  format(
      "for (int i = 0, n = this->_internal_$name$_size(); i < n; i++) {\n"
      "  const auto& s = this->_internal_$name$(i);\n");
  // format("for (const std::string& s : this->$name$()) {\n");
  format.Indent();
  if (descriptor_->type() == FieldDescriptor::TYPE_STRING) {
    GenerateUtf8CheckCodeForString(descriptor_, options_, false,
                                   "s.data(), static_cast<int>(s.length()),\n",
                                   format);
  }
  format.Outdent();
  format(
      "  target = stream->Write$declared_type$($number$, s, target);\n"
      "}\n");
}

void RepeatedStringFieldGenerator::GenerateByteSize(
    io::Printer* printer) const {
  Formatter format(printer, variables_);
  format(
      "total_size += $tag_size$ *\n"
      "    ::$proto_ns$::internal::FromIntSize($field$.size());\n"
      "for (int i = 0, n = $field$.size(); i < n; i++) {\n"
      "  total_size += "
      "::$proto_ns$::internal::WireFormatLite::$declared_type$Size(\n"
      "    $field$.Get(i));\n"
      "}\n");
}

}  // namespace cpp
}  // namespace compiler
}  // namespace protobuf
}  // namespace google
