// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "hpb_generator/gen_repeated_fields.h"

#include <string>
#include <vector>

#include "google/protobuf/descriptor.pb.h"
#include "absl/strings/string_view.h"
#include "hpb_generator/context.h"
#include "hpb_generator/gen_accessors.h"
#include "hpb_generator/gen_enums.h"
#include "hpb_generator/gen_extensions.h"
#include "hpb_generator/gen_utils.h"
#include "hpb_generator/names.h"
#include "google/protobuf/descriptor.h"
#include "upb_generator/c/names.h"
#include "upb_generator/common.h"
#include "upb_generator/file_layout.h"

namespace google {
namespace protobuf {
namespace hpb_generator {

// Adds using accessors to reuse base Access class members from a Proxy/CProxy.
void WriteRepeatedFieldUsingAccessors(const google::protobuf::FieldDescriptor* field,
                                      absl::string_view class_name,
                                      absl::string_view resolved_field_name,
                                      Context& ctx, bool read_only) {
  if (field->cpp_type() == google::protobuf::FieldDescriptor::CPPTYPE_MESSAGE) {
    ctx.Emit({{"class_name", class_name}, {"field_name", resolved_field_name}},
             R"cc(
               using $class_name$Access::$field_name$;
               using $class_name$Access::$field_name$_size;
             )cc");
    if (!read_only) {
      ctx.Emit(
          {{"class_name", class_name}, {"field_name", resolved_field_name}},
          R"cc(
            using $class_name$Access::add_$field_name$;
            using $class_name$Access::add_alias_$field_name$;
            using $class_name$Access::mutable_$field_name$;
          )cc");
    }
  } else {
    ctx.Emit({{"class_name", class_name}, {"field_name", resolved_field_name}},
             R"cc(
               using $class_name$Access::$field_name$;
               using $class_name$Access::$field_name$_size;
             )cc");
    if (!read_only) {
      ctx.Emit(
          {{"class_name", class_name}, {"field_name", resolved_field_name}},
          R"cc(
            using $class_name$Access::add_$field_name$;
            using $class_name$Access::mutable_$field_name$;
            using $class_name$Access::resize_$field_name$;
            using $class_name$Access::set_$field_name$;
          )cc");
    }
  }
}

void WriteRepeatedFieldsInMessageHeader(const google::protobuf::Descriptor* desc,
                                        const google::protobuf::FieldDescriptor* field,
                                        absl::string_view resolved_field_name,
                                        absl::string_view resolved_upbc_name,
                                        Context& ctx) {
  ctx.Emit(
      {{"upb_msg_name", upb::generator::CApiMessageType(desc->full_name())},
       {"field_name", resolved_field_name},
       {"upbc_name", resolved_upbc_name}},
      R"cc(
        inline size_t $field_name$_size() const {
          size_t len;
          $upb_msg_name$_$upbc_name$(msg_, &len);
          return len;
        }
      )cc");

  if (field->cpp_type() == google::protobuf::FieldDescriptor::CPPTYPE_MESSAGE) {
    ctx.Emit(
        {{"mut_ptr_type", MessagePtrConstType(field, /* const */ false)},
         {"const_ptr_type", MessagePtrConstType(field, /* const */ true)},
         {"field_name", resolved_field_name},
         {"upbc_name", resolved_upbc_name},
         {"msg_base_type", MessageBaseType(field, /* maybe_const */ false)}},
        R"cc(
          $const_ptr_type$ $field_name$(size_t index) const;
          const ::hpb::RepeatedField<const $msg_base_type$>::CProxy $field_name$() const;
          ::hpb::Ptr<::hpb::RepeatedField<$msg_base_type$>> mutable_$field_name$();
          absl::StatusOr<$mut_ptr_type$> add_$field_name$();
          /**
           * Re-points submsg of repeated field to given target.
           *
           * REQUIRES: both messages must be in the same arena.
           */
          bool add_alias_$field_name$($mut_ptr_type$ target);
          $mut_ptr_type$ mutable_$field_name$(size_t index) const;
        )cc");
  } else if (field->cpp_type() == google::protobuf::FieldDescriptor::CPPTYPE_STRING) {
    ctx.Emit({{"cpp_const_type", CppConstType(field)},
              {"field_name", resolved_field_name}},
             R"cc(
               $cpp_const_type$ $field_name$(size_t index) const;
               const ::hpb::RepeatedField<$cpp_const_type$>::CProxy $field_name$() const;
               ::hpb::Ptr<::hpb::RepeatedField<$cpp_const_type$>> mutable_$field_name$();
               bool add_$field_name$($cpp_const_type$ val);
               void set_$field_name$(size_t index, $cpp_const_type$ val);
               bool resize_$field_name$(size_t len);
             )cc");
  } else {
    ctx.Emit({{"cpp_const_type", CppConstType(field)},
              {"field_name", resolved_field_name}},
             R"cc(
               $cpp_const_type$ $field_name$(size_t index) const;
               const ::hpb::RepeatedField<$cpp_const_type$>::CProxy $field_name$() const;
               ::hpb::Ptr<::hpb::RepeatedField<$cpp_const_type$>> mutable_$field_name$();
               bool add_$field_name$($cpp_const_type$ val);
               void set_$field_name$(size_t index, $cpp_const_type$ val);
               bool resize_$field_name$(size_t len);
             )cc");
  }
}

void WriteRepeatedMessageAccessor(const google::protobuf::Descriptor* desc,
                                  const google::protobuf::FieldDescriptor* field,
                                  const absl::string_view resolved_field_name,
                                  const absl::string_view class_name,
                                  Context& ctx) {
  const char arena_expression[] = "arena_";
  absl::string_view upbc_name = field->name();
  ctx.Emit(
      {{"class_name", class_name},
       {"const_ptr_type", MessagePtrConstType(field, /* is_const */ true)},
       {"field_name", resolved_field_name},
       {"upb_msg_name", upb::generator::CApiMessageType(desc->full_name())},
       {"msg_base_type", MessageBaseType(field, /* maybe_const */ false)},
       {"upbc_name", upbc_name}},
      R"cc(
        $const_ptr_type$ $class_name$::$field_name$(size_t index) const {
          size_t len;
          auto* ptr = $upb_msg_name$_$upbc_name$(msg_, &len);
          assert(index < len);
          return ::hpb::interop::upb::MakeCHandle<$msg_base_type$>(
              (upb_Message*)*(ptr + index), arena_);
        }
      )cc");
  ctx.Emit(
      {{"class_name", class_name},
       {"mut_ptr_type", MessagePtrConstType(field, /* const */ false)},
       {"field_name", resolved_field_name},
       {"upb_msg_name", upb::generator::CApiMessageType(desc->full_name())},
       {"msg_base_type", MessageBaseType(field, /* maybe_const */ false)},
       {"arena_expr", arena_expression},
       {"upbc_name", upbc_name},
       {"upb_field_msg_name",
        upb::generator::CApiMessageType(field->message_type()->full_name())}},
      R"cc(
        absl::StatusOr<$mut_ptr_type$> $class_name$::add_$field_name$() {
          auto new_msg = $upb_msg_name$_add_$upbc_name$(msg_, $arena_expr$);
          if (!new_msg) {
            return ::hpb::MessageAllocationError();
          }
          return hpb::interop::upb::MakeHandle<$msg_base_type$>(
              (upb_Message*)new_msg, $arena_expr$);
        }

        bool $class_name$::add_alias_$field_name$($mut_ptr_type$ target) {
#ifndef NDEBUG
          ABSL_CHECK(
              upb_Arena_IsFused(arena_, hpb::interop::upb::GetArena(target)) ||
              upb_Arena_HasRef(arena_, hpb::interop::upb::GetArena(target)));
#endif
          size_t size = 0;
          $upb_msg_name$_$field_name$(msg_, &size);
          auto elements = $upb_msg_name$_resize_$field_name$(msg_, size + 1, arena_);
          if (!elements) {
            return false;
          }
          elements[size] = ($upb_field_msg_name$*)hpb::interop::upb::GetMessage(target);
          return true;
        }
      )cc");
  ctx.Emit(
      {{"class_name", class_name},
       {"mut_ptr_type", MessagePtrConstType(field, /* is_const */ false)},
       {"field_name", resolved_field_name},
       {"upb_msg_name", upb::generator::CApiMessageType(desc->full_name())},
       {"msg_base_type", MessageBaseType(field, /* maybe_const */ false)},
       {"arena_expr", arena_expression},
       {"upbc_name", upbc_name}},
      R"cc(
        $mut_ptr_type$ $class_name$::mutable_$field_name$(size_t index) const {
          size_t len;
          auto* ptr = $upb_msg_name$_$upbc_name$(msg_, &len);
          assert(index < len);
          return hpb::interop::upb::MakeHandle<$msg_base_type$>(
              (upb_Message*)*(ptr + index), $arena_expr$);
        }
      )cc");
  ctx.Emit(
      {{"class_name", class_name},
       {"msg_base_type", MessageBaseType(field, /* maybe_const */ false)},
       {"field_name", resolved_field_name},
       {"upb_msg_name", upb::generator::CApiMessageType(desc->full_name())},
       {"upbc_name", upbc_name},
       {"getter_postfix", upb::generator::kRepeatedFieldArrayGetterPostfix},
       {"mutable_getter_postfix",
        upb::generator::kRepeatedFieldMutableArrayGetterPostfix}},
      R"cc(
        const ::hpb::RepeatedField<const $msg_base_type$>::CProxy
        $class_name$::$field_name$() const {
          size_t size;
          const upb_Array* arr =
              _$upb_msg_name$_$upbc_name$_$getter_postfix$(msg_, &size);
          return ::hpb::RepeatedField<const $msg_base_type$>::CProxy(arr, arena_);
        };
        ::hpb::Ptr<::hpb::RepeatedField<$msg_base_type$>>
        $class_name$::mutable_$field_name$() {
          size_t size;
          upb_Array* arr = _$upb_msg_name$_$upbc_name$_$mutable_getter_postfix$(
              msg_, &size, arena_);
          return ::hpb::RepeatedField<$msg_base_type$>::Proxy(arr, arena_);
        }
      )cc");
}

void WriteRepeatedStringAccessor(const google::protobuf::Descriptor* desc,
                                 const google::protobuf::FieldDescriptor* field,
                                 const absl::string_view resolved_field_name,
                                 const absl::string_view class_name,
                                 Context& ctx) {
  absl::string_view upbc_name = field->name();
  ctx.Emit(
      {{"class_name", class_name},
       {"cpp_const_type", CppConstType(field)},
       {"field_name", resolved_field_name},
       {"upb_msg_name", upb::generator::CApiMessageType(desc->full_name())},
       {"upbc_name", upbc_name}},
      R"cc(
        $cpp_const_type$ $class_name$::$field_name$(size_t index) const {
          size_t len;
          auto* ptr = $upb_msg_name$_mutable_$upbc_name$(msg_, &len);
          assert(index < len);
          return hpb::interop::upb::FromUpbStringView(*(ptr + index));
        }
      )cc");
  ctx.Emit(
      {{"class_name", class_name},
       {"field_name", resolved_field_name},
       {"upb_msg_name", upb::generator::CApiMessageType(desc->full_name())},
       {"upbc_name", upbc_name}},
      R"cc(
        bool $class_name$::resize_$field_name$(size_t len) {
          return $upb_msg_name$_resize_$upbc_name$(msg_, len, arena_);
        }
      )cc");
  ctx.Emit(
      {{"class_name", class_name},
       {"cpp_const_type", CppConstType(field)},
       {"field_name", resolved_field_name},
       {"upb_msg_name", upb::generator::CApiMessageType(desc->full_name())},
       {"upbc_name", upbc_name}},
      R"cc(
        bool $class_name$::add_$field_name$($cpp_const_type$ val) {
          return $upb_msg_name$_add_$upbc_name$(
              msg_, hpb::interop::upb::CopyToUpbStringView(val, arena_),
              arena_);
        }
      )cc");
  ctx.Emit(
      {{"class_name", class_name},
       {"cpp_const_type", CppConstType(field)},
       {"field_name", resolved_field_name},
       {"upb_msg_name", upb::generator::CApiMessageType(desc->full_name())},
       {"upbc_name", upbc_name}},
      R"cc(
        void $class_name$::set_$field_name$(size_t index,
                                            $cpp_const_type$ val) {
          size_t len;
          auto* ptr = $upb_msg_name$_mutable_$upbc_name$(msg_, &len);
          assert(index < len);
          *(ptr + index) = hpb::interop::upb::CopyToUpbStringView(val, arena_);
        }
      )cc");
  ctx.Emit(
      {{"class_name", class_name},
       {"cpp_const_type", CppConstType(field)},
       {"field_name", resolved_field_name},
       {"upb_msg_name", upb::generator::CApiMessageType(desc->full_name())},
       {"upbc_name", upbc_name},
       {"getter_postfix", upb::generator::kRepeatedFieldArrayGetterPostfix},
       {"mutable_getter_postfix",
        upb::generator::kRepeatedFieldMutableArrayGetterPostfix}},
      R"cc(
        const ::hpb::RepeatedField<$cpp_const_type$>::CProxy
        $class_name$::$field_name$() const {
          size_t size;
          const upb_Array* arr =
              _$upb_msg_name$_$upbc_name$_$getter_postfix$(msg_, &size);
          return ::hpb::RepeatedField<$cpp_const_type$>::CProxy(arr, arena_);
        };
        ::hpb::Ptr<::hpb::RepeatedField<$cpp_const_type$>>
        $class_name$::mutable_$field_name$() {
          size_t size;
          upb_Array* arr = _$upb_msg_name$_$upbc_name$_$mutable_getter_postfix$(
              msg_, &size, arena_);
          return ::hpb::RepeatedField<$cpp_const_type$>::Proxy(arr, arena_);
        }
      )cc");
}

void WriteRepeatedScalarAccessor(const google::protobuf::Descriptor* desc,
                                 const google::protobuf::FieldDescriptor* field,
                                 const absl::string_view resolved_field_name,
                                 const absl::string_view class_name,
                                 Context& ctx) {
  absl::string_view upbc_name = field->name();
  ctx.Emit(
      {{"class_name", class_name},
       {"cpp_const_type", CppConstType(field)},
       {"field_name", resolved_field_name},
       {"upb_msg_name", upb::generator::CApiMessageType(desc->full_name())},
       {"upbc_name", upbc_name}},
      R"cc(
        $cpp_const_type$ $class_name$::$field_name$(size_t index) const {
          size_t len;
          auto* ptr = $upb_msg_name$_mutable_$upbc_name$(msg_, &len);
          assert(index < len);
          return *(ptr + index);
        }
      )cc");
  ctx.Emit(
      {{"class_name", class_name},
       {"field_name", resolved_field_name},
       {"upb_msg_name", upb::generator::CApiMessageType(desc->full_name())},
       {"upbc_name", upbc_name}},
      R"cc(
        bool $class_name$::resize_$field_name$(size_t len) {
          return $upb_msg_name$_resize_$upbc_name$(msg_, len, arena_);
        }
      )cc");
  ctx.Emit(
      {{"class_name", class_name},
       {"cpp_const_type", CppConstType(field)},
       {"field_name", resolved_field_name},
       {"upb_msg_name", upb::generator::CApiMessageType(desc->full_name())},
       {"upbc_name", upbc_name}},
      R"cc(
        bool $class_name$::add_$field_name$($cpp_const_type$ val) {
          return $upb_msg_name$_add_$upbc_name$(msg_, val, arena_);
        }
      )cc");
  ctx.Emit(
      {{"class_name", class_name},
       {"cpp_const_type", CppConstType(field)},
       {"field_name", resolved_field_name},
       {"upb_msg_name", upb::generator::CApiMessageType(desc->full_name())},
       {"upbc_name", upbc_name}},
      R"cc(
        void $class_name$::set_$field_name$(size_t index,
                                            $cpp_const_type$ val) {
          size_t len;
          auto* ptr = $upb_msg_name$_mutable_$upbc_name$(msg_, &len);
          assert(index < len);
          *(ptr + index) = val;
        }
      )cc");
  ctx.Emit(
      {{"class_name", class_name},
       {"cpp_const_type", CppConstType(field)},
       {"field_name", resolved_field_name},
       {"upb_msg_name", upb::generator::CApiMessageType(desc->full_name())},
       {"upbc_name", upbc_name},
       {"getter_postfix", upb::generator::kRepeatedFieldArrayGetterPostfix},
       {"mutable_getter_postfix",
        upb::generator::kRepeatedFieldMutableArrayGetterPostfix}},
      R"cc(
        const ::hpb::RepeatedField<$cpp_const_type$>::CProxy
        $class_name$::$field_name$() const {
          size_t size;
          const upb_Array* arr =
              _$upb_msg_name$_$upbc_name$_$getter_postfix$(msg_, &size);
          return ::hpb::RepeatedField<$cpp_const_type$>::CProxy(arr, arena_);
        };
        ::hpb::Ptr<::hpb::RepeatedField<$cpp_const_type$>>
        $class_name$::mutable_$field_name$() {
          size_t size;
          upb_Array* arr = _$upb_msg_name$_$upbc_name$_$mutable_getter_postfix$(
              msg_, &size, arena_);
          return ::hpb::RepeatedField<$cpp_const_type$>::Proxy(arr, arena_);
        }
      )cc");
}

}  // namespace hpb_generator
}  // namespace protobuf
}  // namespace google
