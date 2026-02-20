// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "hpb_generator/gen_accessors.h"

#include <string>

#include "absl/container/flat_hash_map.h"
#include "absl/container/flat_hash_set.h"
#include "absl/strings/ascii.h"
#include "absl/strings/match.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/string_view.h"
#include "hpb_generator/context.h"
#include "hpb_generator/gen_repeated_fields.h"
#include "hpb_generator/gen_utils.h"
#include "hpb_generator/keywords.h"
#include "hpb_generator/names.h"
#include "google/protobuf/descriptor.h"
#include "upb_generator/c/names.h"
#include "upb_generator/minitable/names.h"

namespace google {
namespace protobuf {
namespace hpb_generator {

using NameToFieldDescriptorMap =
    absl::flat_hash_map<absl::string_view, const google::protobuf::FieldDescriptor*>;

void WriteFieldAccessorHazzer(const google::protobuf::Descriptor* desc,
                              const google::protobuf::FieldDescriptor* field,
                              absl::string_view resolved_field_name,
                              absl::string_view resolved_upbc_name,
                              Context& ctx);
void WriteFieldAccessorClear(const google::protobuf::Descriptor* desc,
                             const google::protobuf::FieldDescriptor* field,
                             absl::string_view resolved_field_name,
                             absl::string_view resolved_upbc_name,
                             Context& ctx);
void WriteMapFieldAccessors(const google::protobuf::Descriptor* desc,
                            const google::protobuf::FieldDescriptor* field,
                            absl::string_view resolved_field_name,
                            absl::string_view resolved_upbc_name, Context& ctx);

void WriteMapAccessorDefinitions(const google::protobuf::Descriptor* desc,
                                 const google::protobuf::FieldDescriptor* field,
                                 absl::string_view resolved_field_name,
                                 absl::string_view class_name, Context& ctx);

// Returns C++ class member name by resolving naming conflicts across
// proto field names (such as clear_ prefixes) and keyword collisions.
//
// The Upb C generator prefixes all accessors with package and class names
// avoiding collisions. Therefore we need to use raw field names when calling
// into C accessors but need to fully resolve conflicts for C++ class members.
std::string ResolveFieldName(const google::protobuf::FieldDescriptor* field,
                             const NameToFieldDescriptorMap& field_names);

upb::generator::NameMangler CreateNameMangler(
    const google::protobuf::Descriptor* message) {
  return upb::generator::NameMangler(upb::generator::GetCppFields(message));
}

NameToFieldDescriptorMap CreateFieldNameMap(const google::protobuf::Descriptor* message) {
  NameToFieldDescriptorMap field_names;
  for (int i = 0; i < message->field_count(); i++) {
    const google::protobuf::FieldDescriptor* field = message->field(i);
    field_names.emplace(field->name(), field);
  }
  return field_names;
}

void WriteFieldAccessorsInHeader(const google::protobuf::Descriptor* desc, Context& ctx) {
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
      if (field->cpp_type() == google::protobuf::FieldDescriptor::CPPTYPE_STRING) {
        ctx.Emit({{"field_name", resolved_field_name}},
                 R"cc(
                   absl::string_view $field_name$() const;
                   void set_$field_name$(absl::string_view value);
                 )cc");
      } else if (field->cpp_type() ==
                 google::protobuf::FieldDescriptor::CPPTYPE_MESSAGE) {
        ctx.Emit(
            {{"mut_ptr_type", MessagePtrConstType(field, /* const */ false)},
             {"const_ptr_type", MessagePtrConstType(field, /* const */ true)},
             {"field_name", resolved_field_name}},
            R"cc(
              $const_ptr_type$ $field_name$() const;
              $mut_ptr_type$ mutable_$field_name$();
              /**
               * Re-points submessage to the given target.
               *
               * REQUIRES:
               * - both messages must be in the same arena, or in two
               * fused arenas.
               */
              void set_alias_$field_name$($mut_ptr_type$ target);
            )cc");
      } else {
        ctx.Emit({{"cpp_type", CppConstType(field)},
                  {"field_name", resolved_field_name},
                  {"upb_msg_name",
                   upb::generator::CApiMessageType(desc->full_name())},
                  {"upb_field_name", resolved_upbc_name}},
                 R"cc(
                   inline $cpp_type$ $field_name$() const {
                     return $upb_msg_name$_$upb_field_name$(msg_);
                   }
                   inline void set_$field_name$($cpp_type$ value) {
                     return $upb_msg_name$_set_$upb_field_name$(msg_, value);
                   }
                 )cc");
      }
    }
  }
}

void WriteFieldAccessorHazzer(const google::protobuf::Descriptor* desc,
                              const google::protobuf::FieldDescriptor* field,
                              const absl::string_view resolved_field_name,
                              const absl::string_view resolved_upbc_name,
                              Context& ctx) {
  // Generate hazzer (if any).
  if (field->has_presence()) {
    // Has presence.
    ctx.Emit(
        {{"field_name", resolved_field_name},
         {"upb_msg_name", upb::generator::CApiMessageType(desc->full_name())},
         {"upb_field_name", resolved_upbc_name}},
        R"cc(
          inline bool has_$field_name$() const {
            return $upb_msg_name$_has_$upb_field_name$(msg_);
          }
        )cc");
  }
}

void WriteFieldAccessorClear(const google::protobuf::Descriptor* desc,
                             const google::protobuf::FieldDescriptor* field,
                             const absl::string_view resolved_field_name,
                             const absl::string_view resolved_upbc_name,
                             Context& ctx) {
  if (field->has_presence()) {
    ctx.Emit(
        {{"field_name", resolved_field_name},
         {"upb_field_name", resolved_upbc_name},
         {"upb_msg_name", upb::generator::CApiMessageType(desc->full_name())}},
        R"cc(
          void clear_$field_name$() {
            $upb_msg_name$_clear_$upb_field_name$(msg_);
          }
        )cc");
  }
}

void WriteMapFieldAccessors(const google::protobuf::Descriptor* desc,
                            const google::protobuf::FieldDescriptor* field,
                            const absl::string_view resolved_field_name,
                            const absl::string_view resolved_upbc_name,
                            Context& ctx) {
  const google::protobuf::Descriptor* entry = field->message_type();
  const google::protobuf::FieldDescriptor* key = entry->FindFieldByNumber(1);
  const google::protobuf::FieldDescriptor* val = entry->FindFieldByNumber(2);
  ctx.Emit(
      {{"field_name", resolved_field_name},
       {"upb_msg_name", upb::generator::CApiMessageType(desc->full_name())},
       {"const_key", CppConstType(key)},
       {"upb_field_name", resolved_upbc_name}},
      R"cc(
        inline size_t $field_name$_size() const {
          return $upb_msg_name$_$upb_field_name$_size(msg_);
        }
        inline void clear_$field_name$() {
          $upb_msg_name$_clear_$upb_field_name$(msg_);
        }
        void delete_$field_name$($const_key$ key);
      )cc");

  if (val->cpp_type() == google::protobuf::FieldDescriptor::CPPTYPE_MESSAGE) {
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
    ctx.Emit({{"field_name", resolved_field_name},
              {"const_key", CppConstType(key)},
              {"const_val", CppConstType(val)}},
             R"cc(
               bool set_$field_name$($const_key$ key, $const_val$ value);
               absl::StatusOr<$const_val$> get_$field_name$($const_key$ key);
             )cc");
  }
}

void WriteAccessorsInSource(const google::protobuf::Descriptor* desc, Context& ctx) {
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
      if (field->cpp_type() == google::protobuf::FieldDescriptor::CPPTYPE_MESSAGE) {
        WriteRepeatedMessageAccessor(desc, field, resolved_field_name,
                                     class_name, ctx);
      } else if (field->cpp_type() == google::protobuf::FieldDescriptor::CPPTYPE_STRING) {
        WriteRepeatedStringAccessor(desc, field, resolved_field_name,
                                    class_name, ctx);
      } else {
        WriteRepeatedScalarAccessor(desc, field, resolved_field_name,
                                    class_name, ctx);
      }
    } else {
      // non-repeated field.
      if (field->cpp_type() == google::protobuf::FieldDescriptor::CPPTYPE_STRING) {
        ctx.Emit({{"class_name", class_name},
                  {"cpp_const_type", CppConstType(field)},
                  {"field_name", resolved_field_name},
                  {"upb_msg_name",
                   upb::generator::CApiMessageType(desc->full_name())},
                  {"upb_field_name", resolved_upbc_name}},
                 R"cc(
                   $cpp_const_type$ $class_name$::$field_name$() const {
                     return hpb::interop::upb::FromUpbStringView(
                         $upb_msg_name$_$upb_field_name$(msg_));
                   }
                 )cc");
        // Set string.
        ctx.Emit({{"class_name", class_name},
                  {"cpp_const_type", CppConstType(field)},
                  {"field_name", resolved_field_name},
                  {"upb_field_name", resolved_upbc_name},
                  {"upb_msg_name",
                   upb::generator::CApiMessageType(desc->full_name())},
                  {"arena_expr", arena_expression}},
                 R"cc(
                   void $class_name$::set_$field_name$($cpp_const_type$ value) {
                     $upb_msg_name$_set_$upb_field_name$(
                         msg_, hpb::interop::upb::CopyToUpbStringView(
                                   value, $arena_expr$));
                   }
                 )cc");
      } else if (field->cpp_type() ==
                 google::protobuf::FieldDescriptor::CPPTYPE_MESSAGE) {
        ctx.Emit(
            {{"class_name", class_name},
             {"const_ptr_type",
              MessagePtrConstType(field, /* is_const */ true)},
             {"field_name", resolved_field_name},
             {"upb_msg_name",
              upb::generator::CApiMessageType(desc->full_name())},
             {"msg_base_type", MessageBaseType(field, /* maybe_const */ false)},
             {"upb_field_name", resolved_upbc_name}},
            R"cc(
              $const_ptr_type$ $class_name$::$field_name$() const {
                if (!has_$field_name$()) {
                  return $msg_base_type$::default_instance();
                }
                return ::hpb::interop::upb::MakeCHandle<$msg_base_type$>(
                    (upb_Message*)($upb_msg_name$_$upb_field_name$(msg_)),
                    arena_);
              }
            )cc");

        ctx.Emit(
            {{"class_name", class_name},
             {"mut_ptr_type", MessagePtrConstType(field, /* is_const */ false)},
             {"field_name", resolved_field_name},
             {"upb_msg_name",
              upb::generator::CApiMessageType(desc->full_name())},
             {"msg_base_type", MessageBaseType(field, /* maybe_const */ false)},
             {"upb_field_name", resolved_upbc_name},
             {"arena_expr", arena_expression},
             {"desc_class_name", ClassName(desc)},
             {"layout_index", ctx.GetLayoutIndex(field)}},
            R"cc(
              $mut_ptr_type$ $class_name$::mutable_$field_name$() {
                return hpb::interop::upb::MakeHandle<$msg_base_type$>(
                    (upb_Message*)($upb_msg_name$_mutable_$upb_field_name$(
                        msg_, $arena_expr$)),
                    $arena_expr$);
              }
              void $class_name$::set_alias_$field_name$($mut_ptr_type$ target) {
#ifndef NDEBUG
                ABSL_CHECK(upb_Arena_IsFused(
                               arena_, hpb::interop::upb::GetArena(target)) ||
                           upb_Arena_HasRef(
                               arena_, hpb::interop::upb::GetArena(target)));
#endif
                upb_Message_SetBaseFieldMessage(
                    UPB_UPCAST(msg_),
                    upb_MiniTable_GetFieldByIndex(
                        $desc_class_name$::minitable(), $layout_index$),
                    hpb::interop::upb::GetMessage(target));
              }
            )cc");
      }
    }
  }
  ctx.Emit("\n");
  ctx.Emit("}  // namespace internal\n\n");
}

void WriteMapAccessorDefinitions(const google::protobuf::Descriptor* desc,
                                 const google::protobuf::FieldDescriptor* field,
                                 const absl::string_view resolved_field_name,
                                 const absl::string_view class_name,
                                 Context& ctx) {
  const google::protobuf::Descriptor* entry = field->message_type();
  const google::protobuf::FieldDescriptor* key = entry->FindFieldByNumber(1);
  const google::protobuf::FieldDescriptor* val = entry->FindFieldByNumber(2);
  absl::string_view upbc_name = field->name();
  absl::string_view converted_key_name = "key";
  absl::string_view optional_conversion_code = "";

  if (key->cpp_type() == google::protobuf::FieldDescriptor::CPPTYPE_STRING) {
    // Insert conversion from absl::string_view to upb_StringView.
    // Creates upb_StringView on stack to prevent allocation.
    converted_key_name = "upb_key";
    optional_conversion_code =
        "upb_StringView upb_key = {key.data(), key.size()};\n";
  }
  if (val->cpp_type() == google::protobuf::FieldDescriptor::CPPTYPE_MESSAGE) {
    ctx.Emit(
        {{"class_name", class_name},
         {"field_name", resolved_field_name},
         {"const_key", CppConstType(key)},
         {"const_val_ptr", MessagePtrConstType(val, /* is_const */ true)},
         {"upb_msg_name", upb::generator::CApiMessageType(desc->full_name())},
         {"upb_val_msg_name",
          upb::generator::CApiMessageType(val->message_type()->full_name())},
         {"optional_conversion_code", optional_conversion_code},
         {"converted_key_name", converted_key_name},
         {"upb_field_name", upbc_name},
         {"upb_val_minitable", ::upb::generator::MiniTableMessageVarName(
                                   val->message_type()->full_name())}},
        R"cc(
          bool $class_name$::set_$field_name$($const_key$ key,
                                              $const_val_ptr$ value) {
            upb_Message* clone = upb_Message_DeepClone(
                ::hpb::internal::PrivateAccess::GetInternalMsg(value),
                &$upb_val_minitable$, arena_);
            $optional_conversion_code$return
                $upb_msg_name$_$upb_field_name$_set(msg_, $converted_key_name$,
                                                    ($upb_val_msg_name$*)clone,
                                                    arena_);
          }
        )cc");
    ctx.Emit(
        {{"class_name", class_name},
         {"field_name", resolved_field_name},
         {"const_key", CppConstType(key)},
         {"mut_val_ptr", MessagePtrConstType(val, /* is_const */ false)},
         {"upb_msg_name", upb::generator::CApiMessageType(desc->full_name())},
         {"upb_val_msg_name",
          upb::generator::CApiMessageType(val->message_type()->full_name())},
         {"optional_conversion_code", optional_conversion_code},
         {"converted_key_name", converted_key_name},
         {"upb_field_name", upbc_name},
         {"upb_val_minitable", ::upb::generator::MiniTableMessageVarName(
                                   val->message_type()->full_name())}},
        R"cc(
          bool $class_name$::set_$field_name$($const_key$ key,
                                              $mut_val_ptr$ value) {
            upb_Message* clone = upb_Message_DeepClone(
                ::hpb::internal::PrivateAccess::GetInternalMsg(value),
                &$upb_val_minitable$, arena_);
            $optional_conversion_code$return
                $upb_msg_name$_$upb_field_name$_set(msg_, $converted_key_name$,
                                                    ($upb_val_msg_name$*)clone,
                                                    arena_);
          }
        )cc");
    ctx.Emit(
        {{"class_name", class_name},
         {"field_name", resolved_field_name},
         {"const_key", CppConstType(key)},
         {"const_val_ptr", MessagePtrConstType(val, /* is_const */ true)},
         {"upb_msg_name", upb::generator::CApiMessageType(desc->full_name())},
         {"upb_val_msg_name",
          upb::generator::CApiMessageType(val->message_type()->full_name())},
         {"optional_conversion_code", optional_conversion_code},
         {"converted_key_name", converted_key_name},
         {"upb_field_name", upbc_name}},
        R"cc(
          bool $class_name$::set_alias_$field_name$($const_key$ key,
                                                    $const_val_ptr$ value) {
#ifndef NDEBUG
            ABSL_CHECK(
                upb_Arena_IsFused(arena_, hpb::interop::upb::GetArena(value)) ||
                upb_Arena_HasRef(arena_, hpb::interop::upb::GetArena(value)));
#endif
            $optional_conversion_code$return
                $upb_msg_name$_$upb_field_name$_set(
                    msg_, $converted_key_name$,
                    ($upb_val_msg_name$*)hpb::interop::upb::GetMessage(value),
                    arena_);
          }
        )cc");
    ctx.Emit(
        {{"class_name", class_name},
         {"field_name", resolved_field_name},
         {"const_key", CppConstType(key)},
         {"mut_val_ptr", MessagePtrConstType(val, /* is_const */ false)},
         {"upb_msg_name", upb::generator::CApiMessageType(desc->full_name())},
         {"upb_val_msg_name",
          upb::generator::CApiMessageType(val->message_type()->full_name())},
         {"optional_conversion_code", optional_conversion_code},
         {"converted_key_name", converted_key_name},
         {"upb_field_name", upbc_name}},
        R"cc(
          bool $class_name$::set_alias_$field_name$($const_key$ key,
                                                    $mut_val_ptr$ value) {
#ifndef NDEBUG
            ABSL_CHECK(

                upb_Arena_IsFused(arena_, hpb::interop::upb::GetArena(value)) ||
                upb_Arena_HasRef(arena_, hpb::interop::upb::GetArena(value)));
#endif
            $optional_conversion_code$return
                $upb_msg_name$_$upb_field_name$_set(
                    msg_, $converted_key_name$,
                    ($upb_val_msg_name$*)hpb::interop::upb::GetMessage(value),
                    arena_);
          }
        )cc");
    ctx.Emit(
        {{"class_name", class_name},
         {"field_name", resolved_field_name},
         {"const_key", CppConstType(key)},
         {"const_val_ptr", MessagePtrConstType(val, /* is_const */ true)},
         {"upb_msg_name", upb::generator::CApiMessageType(desc->full_name())},
         {"upb_val_msg_name",
          upb::generator::CApiMessageType(val->message_type()->full_name())},
         {"val_proto_class", QualifiedClassName(val->message_type())},
         {"optional_conversion_code", optional_conversion_code},
         {"converted_key_name", converted_key_name},
         {"upb_field_name", upbc_name}},
        R"cc(
          absl::StatusOr<$const_val_ptr$> $class_name$::get_$field_name$(
              $const_key$ key) {
            $upb_val_msg_name$* msg_value;
            $optional_conversion_code$bool success =
                $upb_msg_name$_$upb_field_name$_get(msg_, $converted_key_name$,
                                                    &msg_value);
            if (success) {
              return ::hpb::interop::upb::MakeCHandle<$val_proto_class$>(
                  UPB_UPCAST(msg_value), arena_);
            }
            return absl::NotFoundError("");
          }
        )cc");
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
    ctx.Emit(
        {{"class_name", class_name},
         {"field_name", resolved_field_name},
         {"const_key", CppConstType(key)},
         {"upb_msg_name", upb::generator::CApiMessageType(desc->full_name())},
         {"optional_conversion_code", optional_conversion_code},
         {"converted_key_name", converted_key_name},
         {"upb_field_name", upbc_name}},
        R"cc(
          void $class_name$::delete_$field_name$($const_key$ key) {
            $optional_conversion_code$$upb_msg_name$_$upb_field_name$_delete(
                msg_, $converted_key_name$);
          }
        )cc");
  } else if (val->cpp_type() == google::protobuf::FieldDescriptor::CPPTYPE_STRING) {
    ctx.Emit(
        {{"class_name", class_name},
         {"field_name", resolved_field_name},
         {"const_key", CppConstType(key)},
         {"const_val", CppConstType(val)},
         {"upb_msg_name", upb::generator::CApiMessageType(desc->full_name())},
         {"optional_conversion_code", optional_conversion_code},
         {"converted_key_name", converted_key_name},
         {"upb_field_name", upbc_name}},
        R"cc(
          bool $class_name$::set_$field_name$($const_key$ key,
                                              $const_val$ value) {
            $optional_conversion_code$return
                $upb_msg_name$_$upb_field_name$_set(
                    msg_, $converted_key_name$,
                    hpb::interop::upb::CopyToUpbStringView(value, arena_),
                    arena_);
          }
        )cc");
    ctx.Emit(
        {{"class_name", class_name},
         {"field_name", resolved_field_name},
         {"const_key", CppConstType(key)},
         {"const_val", CppConstType(val)},
         {"upb_msg_name", upb::generator::CApiMessageType(desc->full_name())},
         {"optional_conversion_code", optional_conversion_code},
         {"converted_key_name", converted_key_name},
         {"upb_field_name", upbc_name}},
        R"cc(
          ::absl::StatusOr<$const_val$> $class_name$::get_$field_name$(
              $const_key$ key) {
            upb_StringView value;
            $optional_conversion_code$bool success =
                $upb_msg_name$_$upb_field_name$_get(msg_, $converted_key_name$,
                                                    &value);
            if (success) {
              return ::absl::string_view(value.data, value.size);
            }
            return ::absl::NotFoundError("");
          }
        )cc");
    ctx.Emit(
        {{"class_name", class_name},
         {"field_name", resolved_field_name},
         {"const_key", CppConstType(key)},
         {"upb_msg_name", upb::generator::CApiMessageType(desc->full_name())},
         {"optional_conversion_code", optional_conversion_code},
         {"converted_key_name", converted_key_name},
         {"upb_field_name", upbc_name}},
        R"cc(
          void $class_name$::delete_$field_name$($const_key$ key) {
            $optional_conversion_code$$upb_msg_name$_$upb_field_name$_delete(
                msg_, $converted_key_name$);
          }
        )cc");
  } else {
    ctx.Emit(
        {{"class_name", class_name},
         {"field_name", resolved_field_name},
         {"const_key", CppConstType(key)},
         {"const_val", CppConstType(val)},
         {"upb_msg_name", upb::generator::CApiMessageType(desc->full_name())},
         {"optional_conversion_code", optional_conversion_code},
         {"converted_key_name", converted_key_name},
         {"upb_field_name", upbc_name}},
        R"cc(
          bool $class_name$::set_$field_name$($const_key$ key,
                                              $const_val$ value) {
            $optional_conversion_code$return
                $upb_msg_name$_$upb_field_name$_set(msg_, $converted_key_name$,
                                                    value, arena_);
          }
        )cc");
    ctx.Emit(
        {{"class_name", class_name},
         {"field_name", resolved_field_name},
         {"const_key", CppConstType(key)},
         {"const_val", CppConstType(val)},
         {"upb_msg_name", upb::generator::CApiMessageType(desc->full_name())},
         {"optional_conversion_code", optional_conversion_code},
         {"converted_key_name", converted_key_name},
         {"upb_field_name", upbc_name}},
        R"cc(
          ::absl::StatusOr<$const_val$> $class_name$::get_$field_name$(
              $const_key$ key) {
            $const_val$ value;
            $optional_conversion_code$bool success =
                $upb_msg_name$_$upb_field_name$_get(msg_, $converted_key_name$,
                                                    &value);
            if (success) {
              return value;
            }
            return ::absl::NotFoundError("");
          }
        )cc");
    ctx.Emit(
        {{"class_name", class_name},
         {"field_name", resolved_field_name},
         {"const_key", CppConstType(key)},
         {"upb_msg_name", upb::generator::CApiMessageType(desc->full_name())},
         {"optional_conversion_code", optional_conversion_code},
         {"converted_key_name", converted_key_name},
         {"upb_field_name", upbc_name}},
        R"cc(
          void $class_name$::delete_$field_name$($const_key$ key) {
            $optional_conversion_code$$upb_msg_name$_$upb_field_name$_delete(
                msg_, $converted_key_name$);
          }
        )cc");
  }
}

void WriteUsingAccessorsInHeader(const google::protobuf::Descriptor* desc,
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
      ctx.Emit(
          {{"class_name", class_name}, {"field_name", resolved_field_name}},
          "using $class_name$Access::has_$field_name$;\n");
      if (!read_only) {
        ctx.Emit(
            {{"class_name", class_name}, {"field_name", resolved_field_name}},
            "using $class_name$Access::clear_$field_name$;\n");
      }
    }
    if (field->is_map()) {
      ctx.Emit(
          {{"class_name", class_name}, {"field_name", resolved_field_name}},
          R"cc(
            using $class_name$Access::$field_name$_size;
            using $class_name$Access::get_$field_name$;
          )cc");
      if (!read_only) {
        ctx.Emit(
            {{"class_name", class_name}, {"field_name", resolved_field_name}},
            R"cc(
              using $class_name$Access::clear_$field_name$;
              using $class_name$Access::delete_$field_name$;
              using $class_name$Access::set_$field_name$;
            )cc");
        // only emit set_alias and get_mutable for maps when value is a message
        if (field->message_type()->FindFieldByNumber(2)->cpp_type() ==
            google::protobuf::FieldDescriptor::CPPTYPE_MESSAGE) {
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
      if (field->cpp_type() == google::protobuf::FieldDescriptor::CPPTYPE_MESSAGE) {
        ctx.Emit({{"class_name", ClassName(desc)},
                  {"field_name", resolved_field_name}},
                 "using $class_name$Access::$field_name$;\n");
        if (!read_only) {
          ctx.Emit(
              {{"class_name", class_name}, {"field_name", resolved_field_name}},
              "using $class_name$Access::mutable_$field_name$;\n");
          ctx.Emit(
              {{"class_name", class_name}, {"field_name", resolved_field_name}},
              "using $class_name$Access::set_alias_$field_name$;\n");
        }
      } else {
        ctx.Emit(
            {{"class_name", class_name}, {"field_name", resolved_field_name}},
            "using $class_name$Access::$field_name$;\n");
        if (!read_only) {
          ctx.Emit(
              {{"class_name", class_name}, {"field_name", resolved_field_name}},
              "using $class_name$Access::set_$field_name$;\n");
        }
      }
    }
  }
  for (int i = 0; i < desc->real_oneof_decl_count(); ++i) {
    const google::protobuf::OneofDescriptor* oneof = desc->oneof_decl(i);
    ctx.Emit({{"class_name", class_name}, {"oneof_name", oneof->name()}},
             "using $class_name$Access::$oneof_name$_case;\n");
    ctx.Emit({{"class_name", class_name},
              {"name_camel_case",
               ToCamelCase(oneof->name(), /*lower_first=*/false)}},
             "using $class_name$Access::$name_camel_case$Case;\n");
    for (int j = 0; j < oneof->field_count(); ++j) {
      const google::protobuf::FieldDescriptor* field = oneof->field(j);
      ctx.Emit({{"class_name", class_name},
                {"field_camel_case",
                 ToCamelCase(field->name(), /*lower_first=*/false)}},
               "using $class_name$Access::k$field_camel_case$;\n");
    }
    ctx.Emit({{"class_name", class_name},
              {"oneof_upper", absl::AsciiStrToUpper(oneof->name())}},
             "using $class_name$Access::$oneof_upper$_NOT_SET;\n");
  }
}

void WriteOneofAccessorsInHeader(const google::protobuf::Descriptor* desc, Context& ctx) {
  // Generate const methods.
  auto indent = ctx.printer().WithIndent();
  std::string class_name = ClassName(desc);
  auto field_names = CreateFieldNameMap(desc);
  for (int i = 0; i < desc->real_oneof_decl_count(); ++i) {
    const google::protobuf::OneofDescriptor* oneof = desc->oneof_decl(i);
    ctx.Emit({{"name_camel_case",
               ToCamelCase(oneof->name(), /*lower_first=*/false)}},
             "enum $name_camel_case$Case {\n");
    for (int j = 0; j < oneof->field_count(); ++j) {
      const google::protobuf::FieldDescriptor* field = oneof->field(j);
      ctx.Emit({{"field_camel_case",
                 ToCamelCase(field->name(), /*lower_first=*/false)},
                {"field_number", field->number()}},
               "  k$field_camel_case$ = $field_number$,\n");
    }
    ctx.Emit({{"oneof_upper", absl::AsciiStrToUpper(oneof->name())}},
             "  $oneof_upper$_NOT_SET = 0,\n");
    ctx.Emit("};\n\n");
    ctx.Emit(
        {{"name_camel_case", ToCamelCase(oneof->name(), /*lower_first=*/false)},
         {"oneof_name", oneof->name()}},
        "$name_camel_case$Case $oneof_name$_case() const {\n");
    for (int j = 0; j < oneof->field_count(); ++j) {
      const google::protobuf::FieldDescriptor* field = oneof->field(j);
      std::string resolved_field_name = ResolveFieldName(field, field_names);
      ctx.Emit({{"field_name", resolved_field_name},
                {"field_camel_case",
                 ToCamelCase(field->name(), /*lower_first=*/false)}},
               "  if (has_$field_name$()) { return k$field_camel_case$; }\n");
    }
    ctx.Emit({{"oneof_upper", absl::AsciiStrToUpper(oneof->name())}},
             "  return $oneof_upper$_NOT_SET;\n");
    ctx.Emit("}\n;");
  }
}

std::string ResolveFieldName(const google::protobuf::FieldDescriptor* field,
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
            (candidate->cpp_type() == google::protobuf::FieldDescriptor::CPPTYPE_STRING &&
             prefix == kClearMethodPrefix) ||
            prefix == kSetMethodPrefix || prefix == kHasMethodPrefix) {
          return absl::StrCat(field_name, "_");
        }
      }
    }
  }
  return ResolveKeywordConflict(field_name);
}

}  // namespace hpb_generator
}  // namespace protobuf
}  // namespace google
