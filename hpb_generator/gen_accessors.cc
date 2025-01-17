// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "google/protobuf/compiler/hpb/gen_accessors.h"

#include <string>

#include "absl/container/flat_hash_set.h"
#include "absl/strings/ascii.h"
#include "absl/strings/match.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/string_view.h"
#include "google/protobuf/compiler/hpb/context.h"
#include "google/protobuf/compiler/hpb/gen_repeated_fields.h"
#include "google/protobuf/compiler/hpb/gen_utils.h"
#include "google/protobuf/compiler/hpb/keywords.h"
#include "google/protobuf/compiler/hpb/names.h"
#include "google/protobuf/descriptor.h"
#include "upb_generator/c/names.h"
#include "upb_generator/minitable/names.h"

namespace google::protobuf::hpb_generator {

namespace protobuf = ::proto2;

using NameToFieldDescriptorMap =
    absl::flat_hash_map<absl::string_view, const protobuf::FieldDescriptor*>;

void WriteFieldAccessorHazzer(const protobuf::Descriptor* desc,
                              const protobuf::FieldDescriptor* field,
                              absl::string_view resolved_field_name,
                              absl::string_view resolved_upbc_name,
                              Context& ctx);
void WriteFieldAccessorClear(const protobuf::Descriptor* desc,
                             const protobuf::FieldDescriptor* field,
                             absl::string_view resolved_field_name,
                             absl::string_view resolved_upbc_name,
                             Context& ctx);
void WriteMapFieldAccessors(const protobuf::Descriptor* desc,
                            const protobuf::FieldDescriptor* field,
                            absl::string_view resolved_field_name,
                            absl::string_view resolved_upbc_name, Context& ctx);

void WriteMapAccessorDefinitions(const protobuf::Descriptor* message,
                                 const protobuf::FieldDescriptor* field,
                                 absl::string_view resolved_field_name,
                                 absl::string_view class_name, Context& ctx);

// Returns C++ class member name by resolving naming conflicts across
// proto field names (such as clear_ prefixes) and keyword collisions.
//
// The Upb C generator prefixes all accessors with package and class names
// avoiding collisions. Therefore we need to use raw field names when calling
// into C accessors but need to fully resolve conflicts for C++ class members.
std::string ResolveFieldName(const protobuf::FieldDescriptor* field,
                             const NameToFieldDescriptorMap& field_names);

upb::generator::NameMangler CreateNameMangler(
    const protobuf::Descriptor* message) {
  return upb::generator::NameMangler(upb::generator::GetCppFields(message));
}

NameToFieldDescriptorMap CreateFieldNameMap(
    const protobuf::Descriptor* message) {
  NameToFieldDescriptorMap field_names;
  for (int i = 0; i < message->field_count(); i++) {
    const protobuf::FieldDescriptor* field = message->field(i);
    field_names.emplace(field->name(), field);
  }
  return field_names;
}

void WriteFieldAccessorsInHeader(const protobuf::Descriptor* desc,
                                 Context& ctx) {
  // Generate const methods.
  auto field_names = CreateFieldNameMap(desc);
  auto mangler = CreateNameMangler(desc);

  auto indent = ctx.printer().WithIndent();

  for (const auto* field : FieldNumberOrder(desc)) {
    std::string resolved_field_name = ResolveFieldName(field, field_names);
    std::string resolved_upbc_name = mangler.ResolveFieldName(field->name());
    WriteFieldAccessorHazzer(desc, field, resolved_field_name,
                             resolved_upbc_name, ctx);
    WriteFieldAccessorClear(desc, field, resolved_field_name,
                            resolved_upbc_name, ctx);

    if (field->is_map()) {
      WriteMapFieldAccessors(desc, field, resolved_field_name,
                             resolved_upbc_name, ctx);
    } else if (desc->options().map_entry()) {
      // TODO Implement map entry
    } else if (field->is_repeated()) {
      WriteRepeatedFieldsInMessageHeader(desc, field, resolved_field_name,
                                         resolved_upbc_name, ctx);
    } else {
      // non-repeated.
      if (field->cpp_type() == protobuf::FieldDescriptor::CPPTYPE_STRING) {
        ctx.EmitLegacy(R"cc(
                         $0 $1() const;
                         void set_$1($0 value);
                       )cc",
                       CppConstType(field), resolved_field_name);
      } else if (field->cpp_type() ==
                 protobuf::FieldDescriptor::CPPTYPE_MESSAGE) {
        ctx.EmitLegacy(R"cc(
                         $1 $2() const;
                         $0 mutable_$2();
                         /**
                          * Re-points submessage to the given target.
                          *
                          * REQUIRES:
                          * - both messages must be in the same arena, or in two
                          * fused arenas.
                          */
                         void set_alias_$2($0 target);
                       )cc",
                       MessagePtrConstType(field, /* const */ false),
                       MessagePtrConstType(field, /* const */ true),
                       resolved_field_name);
      } else {
        ctx.EmitLegacy(
            R"cc(
              inline $0 $1() const { return $2_$3(msg_); }
              inline void set_$1($0 value) { return $2_set_$3(msg_, value); }
            )cc",
            CppConstType(field), resolved_field_name,
            upb::generator::CApiMessageType(desc->full_name()),
            resolved_upbc_name);
      }
    }
  }
}

void WriteFieldAccessorHazzer(const protobuf::Descriptor* desc,
                              const protobuf::FieldDescriptor* field,
                              const absl::string_view resolved_field_name,
                              const absl::string_view resolved_upbc_name,
                              Context& ctx) {
  // Generate hazzer (if any).
  if (field->has_presence()) {
    // Has presence.
    ctx.EmitLegacy("inline bool has_$0() const { return $1_has_$2(msg_); }\n",
                   resolved_field_name,
                   upb::generator::CApiMessageType(desc->full_name()),
                   resolved_upbc_name);
  }
}

void WriteFieldAccessorClear(const protobuf::Descriptor* desc,
                             const protobuf::FieldDescriptor* field,
                             const absl::string_view resolved_field_name,
                             const absl::string_view resolved_upbc_name,
                             Context& ctx) {
  if (field->has_presence()) {
    ctx.EmitLegacy("void clear_$0() { $2_clear_$1(msg_); }\n",
                   resolved_field_name, resolved_upbc_name,
                   upb::generator::CApiMessageType(desc->full_name()));
  }
}

void WriteMapFieldAccessors(const protobuf::Descriptor* desc,
                            const protobuf::FieldDescriptor* field,
                            const absl::string_view resolved_field_name,
                            const absl::string_view resolved_upbc_name,
                            Context& ctx) {
  const protobuf::Descriptor* entry = field->message_type();
  const protobuf::FieldDescriptor* key = entry->FindFieldByNumber(1);
  const protobuf::FieldDescriptor* val = entry->FindFieldByNumber(2);
  ctx.EmitLegacy(
      R"cc(
        inline size_t $0_size() const { return $1_$3_size(msg_); }
        inline void clear_$0() { $1_clear_$3(msg_); }
        void delete_$0($2 key);
      )cc",
      resolved_field_name, upb::generator::CApiMessageType(desc->full_name()),
      CppConstType(key), resolved_upbc_name);

  if (val->cpp_type() == protobuf::FieldDescriptor::CPPTYPE_MESSAGE) {
    ctx.Emit({{"field_name", resolved_field_name},
              {"const_key", CppConstType(key)},
              {"const_val", CppConstType(val)},
              {"ConstPtr", MessagePtrConstType(val, true)},
              {"MutPtr", MessagePtrConstType(val, false)}},
             R"cc(
               bool set_$field_name$($const_key$ key, $ConstPtr$ value);
               bool set_$field_name$($const_key$ key, $MutPtr$ value);
               bool set_alias_$field_name$($const_key$ key, $ConstPtr$ value);
               bool set_alias_$field_name$($const_key$ key, $MutPtr$ value);
               absl::StatusOr<$ConstPtr$> get_$field_name$($const_key$ key);
               absl::StatusOr<$MutPtr$> get_mutable_$field_name$($const_key$ key);
             )cc");
  } else {
    ctx.EmitLegacy(
        R"cc(
          bool set_$0($1 key, $2 value);
          absl::StatusOr<$2> get_$0($1 key);
        )cc",
        resolved_field_name, CppConstType(key), CppConstType(val));
  }
}

void WriteAccessorsInSource(const protobuf::Descriptor* desc, Context& ctx) {
  std::string class_name = ClassName(desc);
  absl::StrAppend(&class_name, "Access");
  ctx.Emit("namespace internal {\n");
  const char arena_expression[] = "arena_";
  auto field_names = CreateFieldNameMap(desc);
  auto mangler = CreateNameMangler(desc);

  // Generate const methods.
  auto indent = ctx.printer().WithIndent();
  for (const auto* field : FieldNumberOrder(desc)) {
    std::string resolved_field_name = ResolveFieldName(field, field_names);
    std::string resolved_upbc_name = mangler.ResolveFieldName(field->name());
    if (field->is_map()) {
      WriteMapAccessorDefinitions(desc, field, resolved_field_name, class_name,
                                  ctx);
    } else if (desc->options().map_entry()) {
      // TODO Implement map entry
    } else if (field->is_repeated()) {
      if (field->cpp_type() == protobuf::FieldDescriptor::CPPTYPE_MESSAGE) {
        WriteRepeatedMessageAccessor(desc, field, resolved_field_name,
                                     class_name, ctx);
      } else if (field->cpp_type() ==
                 protobuf::FieldDescriptor::CPPTYPE_STRING) {
        WriteRepeatedStringAccessor(desc, field, resolved_field_name,
                                    class_name, ctx);
      } else {
        WriteRepeatedScalarAccessor(desc, field, resolved_field_name,
                                    class_name, ctx);
      }
    } else {
      // non-repeated field.
      if (field->cpp_type() == protobuf::FieldDescriptor::CPPTYPE_STRING) {
        ctx.EmitLegacy(
            R"cc(
              $1 $0::$2() const {
                return hpb::interop::upb::FromUpbStringView($3_$4(msg_));
              }
            )cc",
            class_name, CppConstType(field), resolved_field_name,
            upb::generator::CApiMessageType(desc->full_name()),
            resolved_upbc_name);
        // Set string.
        ctx.EmitLegacy(
            R"cc(
              void $0::set_$2($1 value) {
                $4_set_$3(msg_, hpb::interop::upb::CopyToUpbStringView(value, $5));
              }
            )cc",
            class_name, CppConstType(field), resolved_field_name,
            resolved_upbc_name,
            upb::generator::CApiMessageType(desc->full_name()),
            arena_expression);
      } else if (field->cpp_type() ==
                 protobuf::FieldDescriptor::CPPTYPE_MESSAGE) {
        ctx.EmitLegacy(
            R"cc(
              $1 $0::$2() const {
                if (!has_$2()) {
                  return $4::default_instance();
                }
                return ::hpb::interop::upb::MakeCHandle<$4>(
                    (upb_Message*)($3_$5(msg_)), arena_);
              }
            )cc",
            class_name, MessagePtrConstType(field, /* is_const */ true),
            resolved_field_name,
            upb::generator::CApiMessageType(desc->full_name()),
            MessageBaseType(field, /* maybe_const */ false),
            resolved_upbc_name);

        ctx.EmitLegacy(
            R"cc(
              $1 $0::mutable_$2() {
                return hpb::interop::upb::MakeHandle<$4>(
                    (upb_Message*)($3_mutable_$5(msg_, $6)), $6);
              }
              void $0::set_alias_$2($1 target) {
                ABSL_CHECK(upb_Arena_IsFused(arena_, hpb::interop::upb::GetArena(target)));
                upb_Message_SetBaseFieldMessage(
                    UPB_UPCAST(msg_),
                    upb_MiniTable_GetFieldByIndex($7::minitable(), $8),
                    hpb::interop::upb::GetMessage(target));
              }
            )cc",
            class_name, MessagePtrConstType(field, /* is_const */ false),
            resolved_field_name,
            upb::generator::CApiMessageType(desc->full_name()),
            MessageBaseType(field, /* maybe_const */ false), resolved_upbc_name,
            arena_expression, ClassName(desc), ctx.GetLayoutIndex(field));
      }
    }
  }
  ctx.Emit("\n");
  ctx.Emit("}  // namespace internal\n\n");
}

void WriteMapAccessorDefinitions(const protobuf::Descriptor* desc,
                                 const protobuf::FieldDescriptor* field,
                                 const absl::string_view resolved_field_name,
                                 const absl::string_view class_name,
                                 Context& ctx) {
  const protobuf::Descriptor* entry = field->message_type();
  const protobuf::FieldDescriptor* key = entry->FindFieldByNumber(1);
  const protobuf::FieldDescriptor* val = entry->FindFieldByNumber(2);
  absl::string_view upbc_name = field->name();
  absl::string_view converted_key_name = "key";
  absl::string_view optional_conversion_code = "";

  if (key->cpp_type() == protobuf::FieldDescriptor::CPPTYPE_STRING) {
    // Insert conversion from absl::string_view to upb_StringView.
    // Creates upb_StringView on stack to prevent allocation.
    converted_key_name = "upb_key";
    optional_conversion_code =
        "upb_StringView upb_key = {key.data(), key.size()};\n";
  }
  if (val->cpp_type() == protobuf::FieldDescriptor::CPPTYPE_MESSAGE) {
    ctx.EmitLegacy(
        R"cc(
          bool $0::set_$1($2 key, $3 value) {
            upb_Message* clone = upb_Message_DeepClone(
                ::hpb::internal::PrivateAccess::GetInternalMsg(value), &$9,
                arena_);
            $6return $4_$8_set(msg_, $7, ($5*)clone, arena_);
          }
        )cc",
        class_name, resolved_field_name, CppConstType(key),
        MessagePtrConstType(val, /* is_const */ true),
        upb::generator::CApiMessageType(desc->full_name()),
        upb::generator::CApiMessageType(val->message_type()->full_name()),
        optional_conversion_code, converted_key_name, upbc_name,
        ::upb::generator::MiniTableMessageVarName(
            val->message_type()->full_name()));
    ctx.EmitLegacy(
        R"cc(
          bool $0::set_$1($2 key, $3 value) {
            upb_Message* clone = upb_Message_DeepClone(
                ::hpb::internal::PrivateAccess::GetInternalMsg(value), &$9,
                arena_);
            $6return $4_$8_set(msg_, $7, ($5*)clone, arena_);
          }
        )cc",
        class_name, resolved_field_name, CppConstType(key),
        MessagePtrConstType(val, /* is_const */ false),
        upb::generator::CApiMessageType(desc->full_name()),
        upb::generator::CApiMessageType(val->message_type()->full_name()),
        optional_conversion_code, converted_key_name, upbc_name,
        ::upb::generator::MiniTableMessageVarName(
            val->message_type()->full_name()));
    ctx.EmitLegacy(
        R"cc(
          bool $0::set_alias_$1($2 key, $3 value) {
            $6return $4_$8_set(
                msg_, $7, ($5*)hpb::interop::upb::GetMessage(value), arena_);
          }
        )cc",
        class_name, resolved_field_name, CppConstType(key),
        MessagePtrConstType(val, /* is_const */ true),
        upb::generator::CApiMessageType(desc->full_name()),
        upb::generator::CApiMessageType(val->message_type()->full_name()),
        optional_conversion_code, converted_key_name, upbc_name);
    ctx.EmitLegacy(
        R"cc(
          bool $0::set_alias_$1($2 key, $3 value) {
            $6return $4_$8_set(
                msg_, $7, ($5*)hpb::interop::upb::GetMessage(value), arena_);
          }
        )cc",
        class_name, resolved_field_name, CppConstType(key),
        MessagePtrConstType(val, /* is_const */ false),
        upb::generator::CApiMessageType(desc->full_name()),
        upb::generator::CApiMessageType(val->message_type()->full_name()),
        optional_conversion_code, converted_key_name, upbc_name);
    ctx.EmitLegacy(
        R"cc(
          absl::StatusOr<$3> $0::get_$1($2 key) {
            $5* msg_value;
            $7bool success = $4_$9_get(msg_, $8, &msg_value);
            if (success) {
              return ::hpb::interop::upb::MakeCHandle<$6>(UPB_UPCAST(msg_value), arena_);
            }
            return absl::NotFoundError("");
          }
        )cc",
        class_name, resolved_field_name, CppConstType(key),
        MessagePtrConstType(val, /* is_const */ true),
        upb::generator::CApiMessageType(desc->full_name()),
        upb::generator::CApiMessageType(val->message_type()->full_name()),
        QualifiedClassName(val->message_type()), optional_conversion_code,
        converted_key_name, upbc_name);
    ctx.Emit(
        {{"class_name", class_name},
         {"hpb_field_name", resolved_field_name},
         {"const_key", CppConstType(key)},
         {"PtrMut", MessagePtrConstType(val, false)},
         {"upb_msg_name", upb::generator::CApiMessageType(desc->full_name())},
         {"return_type",
          upb::generator::CApiMessageType(val->message_type()->full_name())},
         {"proto_class", QualifiedClassName(val->message_type())},
         {"optional_conversion_code", optional_conversion_code},
         {"converted_key_name", converted_key_name},
         {"upb_field_name", upbc_name}},
        R"cc(
          absl::StatusOr<$PtrMut$> $class_name$::get_mutable_$hpb_field_name$(
              $const_key$ key) {
            $return_type$* msg_value;
            $optional_conversion_code$bool success =
                $upb_msg_name$_$upb_field_name$_get(msg_, $converted_key_name$,
                                                    &msg_value);
            if (success) {
              return ::hpb::interop::upb::MakeHandle<$proto_class$>(
                  UPB_UPCAST(msg_value), arena_);
            }
            return absl::NotFoundError("");
          }
        )cc");
    ctx.EmitLegacy(
        R"cc(
          void $0::delete_$1($2 key) { $6$4_$8_delete(msg_, $7); }
        )cc",
        class_name, resolved_field_name, CppConstType(key),
        MessagePtrConstType(val, /* is_const */ false),
        upb::generator::CApiMessageType(desc->full_name()),
        upb::generator::CApiMessageType(val->message_type()->full_name()),
        optional_conversion_code, converted_key_name, upbc_name);
  } else if (val->cpp_type() == protobuf::FieldDescriptor::CPPTYPE_STRING) {
    ctx.EmitLegacy(
        R"cc(
          bool $0::set_$1($2 key, $3 value) {
            $5return $4_$7_set(
                msg_, $6, hpb::interop::upb::CopyToUpbStringView(value, arena_),
                arena_);
          }
        )cc",
        class_name, resolved_field_name, CppConstType(key), CppConstType(val),
        upb::generator::CApiMessageType(desc->full_name()),
        optional_conversion_code, converted_key_name, upbc_name);
    ctx.EmitLegacy(
        R"cc(
          absl::StatusOr<$3> $0::get_$1($2 key) {
            upb_StringView value;
            $5bool success = $4_$7_get(msg_, $6, &value);
            if (success) {
              return absl::string_view(value.data, value.size);
            }
            return absl::NotFoundError("");
          }
        )cc",
        class_name, resolved_field_name, CppConstType(key), CppConstType(val),
        upb::generator::CApiMessageType(desc->full_name()),
        optional_conversion_code, converted_key_name, upbc_name);
    ctx.EmitLegacy(
        R"cc(
          void $0::delete_$1($2 key) { $5$4_$7_delete(msg_, $6); }
        )cc",
        class_name, resolved_field_name, CppConstType(key), CppConstType(val),
        upb::generator::CApiMessageType(desc->full_name()),
        optional_conversion_code, converted_key_name, upbc_name);
  } else {
    ctx.EmitLegacy(
        R"cc(
          bool $0::set_$1($2 key, $3 value) {
            $5return $4_$7_set(msg_, $6, value, arena_);
          }
        )cc",
        class_name, resolved_field_name, CppConstType(key), CppConstType(val),
        upb::generator::CApiMessageType(desc->full_name()),
        optional_conversion_code, converted_key_name, upbc_name);
    ctx.EmitLegacy(
        R"cc(
          absl::StatusOr<$3> $0::get_$1($2 key) {
            $3 value;
            $5bool success = $4_$7_get(msg_, $6, &value);
            if (success) {
              return value;
            }
            return absl::NotFoundError("");
          }
        )cc",
        class_name, resolved_field_name, CppConstType(key), CppConstType(val),
        upb::generator::CApiMessageType(desc->full_name()),
        optional_conversion_code, converted_key_name, upbc_name);
    ctx.EmitLegacy(
        R"cc(
          void $0::delete_$1($2 key) { $5$4_$7_delete(msg_, $6); }
        )cc",
        class_name, resolved_field_name, CppConstType(key), CppConstType(val),
        upb::generator::CApiMessageType(desc->full_name()),
        optional_conversion_code, converted_key_name, upbc_name);
  }
}

void WriteUsingAccessorsInHeader(const protobuf::Descriptor* desc,
                                 MessageClassType handle_type, Context& ctx) {
  bool read_only = handle_type == MessageClassType::kMessageCProxy;

  // Generate const methods.
  auto indent = ctx.printer().WithIndent();
  std::string class_name = ClassName(desc);
  auto field_names = CreateFieldNameMap(desc);

  for (const auto* field : FieldNumberOrder(desc)) {
    std::string resolved_field_name = ResolveFieldName(field, field_names);
    // Generate hazzer (if any).
    if (field->has_presence()) {
      ctx.EmitLegacy("using $0Access::has_$1;\n", class_name,
                     resolved_field_name);
      if (!read_only) {
        ctx.EmitLegacy("using $0Access::clear_$1;\n", class_name,
                       resolved_field_name);
      }
    }
    if (field->is_map()) {
      ctx.EmitLegacy(
          R"cc(
            using $0Access::$1_size;
            using $0Access::get_$1;
          )cc",
          class_name, resolved_field_name);
      if (!read_only) {
        ctx.EmitLegacy(
            R"cc(
              using $0Access::clear_$1;
              using $0Access::delete_$1;
              using $0Access::set_$1;
            )cc",
            class_name, resolved_field_name);
        // only emit set_alias and get_mutable for maps when value is a message
        if (field->message_type()->FindFieldByNumber(2)->cpp_type() ==
            protobuf::FieldDescriptor::CPPTYPE_MESSAGE) {
          ctx.Emit(
              {{"class_name", class_name}, {"field_name", resolved_field_name}},
              R"cc(
                using $class_name$Access::get_mutable_$field_name$;
                using $class_name$Access::set_alias_$field_name$;
              )cc");
        }
      }
    } else if (desc->options().map_entry()) {
      // TODO Implement map entry
    } else if (field->is_repeated()) {
      WriteRepeatedFieldUsingAccessors(field, class_name, resolved_field_name,
                                       ctx, read_only);
    } else {
      if (field->cpp_type() == protobuf::FieldDescriptor::CPPTYPE_MESSAGE) {
        ctx.EmitLegacy("using $0Access::$1;\n", ClassName(desc),
                       resolved_field_name);
        if (!read_only) {
          ctx.EmitLegacy("using $0Access::mutable_$1;\n", class_name,
                         resolved_field_name);
          ctx.EmitLegacy("using $0Access::set_alias_$1;\n", class_name,
                         resolved_field_name);
        }
      } else {
        ctx.EmitLegacy("using $0Access::$1;\n", class_name,
                       resolved_field_name);
        if (!read_only) {
          ctx.EmitLegacy("using $0Access::set_$1;\n", class_name,
                         resolved_field_name);
        }
      }
    }
  }
  for (int i = 0; i < desc->real_oneof_decl_count(); ++i) {
    const protobuf::OneofDescriptor* oneof = desc->oneof_decl(i);
    ctx.EmitLegacy("using $0Access::$1_case;\n", class_name, oneof->name());
    ctx.EmitLegacy("using $0Access::$1Case;\n", class_name,
                   ToCamelCase(oneof->name(), /*lower_first=*/false));
    for (int j = 0; j < oneof->field_count(); ++j) {
      const protobuf::FieldDescriptor* field = oneof->field(j);
      ctx.EmitLegacy("using $0Access::k$1;\n", class_name,
                     ToCamelCase(field->name(), /*lower_first=*/false),
                     field->number());
    }
    ctx.EmitLegacy("using $0Access::$1_NOT_SET;\n", class_name,
                   absl::AsciiStrToUpper(oneof->name()));
  }
}

void WriteOneofAccessorsInHeader(const protobuf::Descriptor* desc,
                                 Context& ctx) {
  // Generate const methods.
  auto indent = ctx.printer().WithIndent();
  std::string class_name = ClassName(desc);
  auto field_names = CreateFieldNameMap(desc);
  for (int i = 0; i < desc->real_oneof_decl_count(); ++i) {
    const protobuf::OneofDescriptor* oneof = desc->oneof_decl(i);
    ctx.EmitLegacy("enum $0Case {\n",
                   ToCamelCase(oneof->name(), /*lower_first=*/false));
    for (int j = 0; j < oneof->field_count(); ++j) {
      const protobuf::FieldDescriptor* field = oneof->field(j);
      ctx.EmitLegacy("  k$0 = $1,\n",
                     ToCamelCase(field->name(), /*lower_first=*/false),
                     field->number());
    }
    ctx.EmitLegacy("  $0_NOT_SET = 0,\n", absl::AsciiStrToUpper(oneof->name()));
    ctx.Emit("};\n\n");
    ctx.EmitLegacy("$0Case $1_case() const {\n",
                   ToCamelCase(oneof->name(), /*lower_first=*/false),
                   oneof->name());
    for (int j = 0; j < oneof->field_count(); ++j) {
      const protobuf::FieldDescriptor* field = oneof->field(j);
      std::string resolved_field_name = ResolveFieldName(field, field_names);
      ctx.EmitLegacy("  if (has_$0()) { return k$1; }\n", resolved_field_name,
                     ToCamelCase(field->name(), /*lower_first=*/false));
    }
    ctx.EmitLegacy("  return $0_NOT_SET;\n",
                   absl::AsciiStrToUpper(oneof->name()));
    ctx.Emit("}\n;");
  }
}

std::string ResolveFieldName(const protobuf::FieldDescriptor* field,
                             const NameToFieldDescriptorMap& field_names) {
  // C++ implementation specific reserved names.
  static const auto& kReservedNames =
      *new absl::flat_hash_set<absl::string_view>({
          "msg",
          "msg_",
          "arena",
          "arena_",
      });

  // C++ specific prefixes used by code generator for field access.
  static constexpr absl::string_view kClearMethodPrefix = "clear_";
  static constexpr absl::string_view kSetMethodPrefix = "set_";
  static constexpr absl::string_view kHasMethodPrefix = "has_";
  static constexpr absl::string_view kDeleteMethodPrefix = "delete_";
  static constexpr absl::string_view kAddToRepeatedMethodPrefix = "add_";
  static constexpr absl::string_view kResizeArrayMethodPrefix = "resize_";

  // List of generated accessor prefixes to check against.
  // Example:
  //     optional repeated string phase = 236;
  //     optional bool clear_phase = 237;
  static constexpr absl::string_view kAccessorPrefixes[] = {
      kClearMethodPrefix,       kDeleteMethodPrefix, kAddToRepeatedMethodPrefix,
      kResizeArrayMethodPrefix, kSetMethodPrefix,    kHasMethodPrefix};

  absl::string_view field_name = field->name();
  if (kReservedNames.count(field_name) > 0) {
    if (absl::EndsWith(field_name, "_")) {
      return absl::StrCat(field_name, "_");
    } else {
      return absl::StrCat(field_name, "__");
    }
  }
  for (const auto prefix : kAccessorPrefixes) {
    // If field name starts with a prefix such as clear_ and the proto
    // contains a field name with trailing end, depending on type of field
    // (repeated, map, message) we have a conflict to resolve.
    if (absl::StartsWith(field_name, prefix)) {
      auto match = field_names.find(field_name.substr(prefix.size()));
      if (match != field_names.end()) {
        const auto* candidate = match->second;
        if (candidate->is_repeated() || candidate->is_map() ||
            (candidate->cpp_type() ==
                 protobuf::FieldDescriptor::CPPTYPE_STRING &&
             prefix == kClearMethodPrefix) ||
            prefix == kSetMethodPrefix || prefix == kHasMethodPrefix) {
          return absl::StrCat(field_name, "_");
        }
      }
    }
  }
  return ResolveKeywordConflict(field_name);
}

}  // namespace protobuf
}  // namespace google::hpb_generator
