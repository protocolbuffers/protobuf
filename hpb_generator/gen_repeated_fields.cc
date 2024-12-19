// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "google/protobuf/compiler/hpb/gen_repeated_fields.h"

#include <string>
#include <vector>

#include "google/protobuf/descriptor.pb.h"
#include "absl/strings/string_view.h"
#include "google/protobuf/compiler/hpb/context.h"
#include "google/protobuf/compiler/hpb/gen_accessors.h"
#include "google/protobuf/compiler/hpb/gen_enums.h"
#include "google/protobuf/compiler/hpb/gen_extensions.h"
#include "google/protobuf/compiler/hpb/gen_utils.h"
#include "google/protobuf/compiler/hpb/names.h"
#include "google/protobuf/descriptor.h"
#include "upb_generator/c/names.h"
#include "upb_generator/common.h"
#include "upb_generator/file_layout.h"

namespace google::protobuf::hpb_generator {

namespace protobuf = ::proto2;

// Adds using accessors to reuse base Access class members from a Proxy/CProxy.
void WriteRepeatedFieldUsingAccessors(const protobuf::FieldDescriptor* field,
                                      absl::string_view class_name,
                                      absl::string_view resolved_field_name,
                                      Context& ctx, bool read_only) {
  if (field->cpp_type() == protobuf::FieldDescriptor::CPPTYPE_MESSAGE) {
    ctx.EmitLegacy(
        R"cc(
          using $0Access::$1;
          using $0Access::$1_size;
        )cc",
        class_name, resolved_field_name);
    if (!read_only) {
      ctx.EmitLegacy(
          R"cc(
            using $0Access::add_$1;
            using $0Access::add_alias_$1;
            using $0Access::mutable_$1;
          )cc",
          class_name, resolved_field_name);
    }
  } else {
    ctx.EmitLegacy(
        R"cc(
          using $0Access::$1;
          using $0Access::$1_size;
        )cc",
        class_name, resolved_field_name);
    if (!read_only) {
      ctx.EmitLegacy(
          R"cc(
            using $0Access::add_$1;
            using $0Access::mutable_$1;
            using $0Access::resize_$1;
            using $0Access::set_$1;
          )cc",
          class_name, resolved_field_name);
    }
  }
}

void WriteRepeatedFieldsInMessageHeader(const protobuf::Descriptor* desc,
                                        const protobuf::FieldDescriptor* field,
                                        absl::string_view resolved_field_name,
                                        absl::string_view resolved_upbc_name,
                                        Context& ctx) {
  ctx.EmitLegacy(
      R"cc(
        inline size_t $1_size() const {
          size_t len;
          $0_$2(msg_, &len);
          return len;
        }
      )cc",
      upb::generator::CApiMessageType(desc->full_name()), resolved_field_name,
      resolved_upbc_name);

  if (field->cpp_type() == protobuf::FieldDescriptor::CPPTYPE_MESSAGE) {
    ctx.EmitLegacy(
        R"cc(
          $1 $2(size_t index) const;
          const ::hpb::RepeatedField<const $4>::CProxy $2() const;
          ::hpb::Ptr<::hpb::RepeatedField<$4>> mutable_$2();
          absl::StatusOr<$0> add_$2();
          /**
           * Re-points submsg of repeated field to given target.
           *
           * REQUIRES: both messages must be in the same arena.
           */
          bool add_alias_$2($0 target);
          $0 mutable_$2(size_t index) const;
        )cc",
        MessagePtrConstType(field, /* const */ false),   // $0
        MessagePtrConstType(field, /* const */ true),    // $1
        resolved_field_name,                             // $2
        resolved_upbc_name,                              // $3
        MessageBaseType(field, /* maybe_const */ false)  // $4
    );
  } else if (field->cpp_type() == protobuf::FieldDescriptor::CPPTYPE_STRING) {
    ctx.EmitLegacy(
        R"cc(
          $0 $1(size_t index) const;
          const ::hpb::RepeatedField<$0>::CProxy $1() const;
          ::hpb::Ptr<::hpb::RepeatedField<$0>> mutable_$1();
          bool add_$1($0 val);
          void set_$1(size_t index, $0 val);
          bool resize_$1(size_t len);
        )cc",
        CppConstType(field), resolved_field_name);
  } else {
    ctx.EmitLegacy(
        R"cc(
          $0 $1(size_t index) const;
          const ::hpb::RepeatedField<$0>::CProxy $1() const;
          ::hpb::Ptr<::hpb::RepeatedField<$0>> mutable_$1();
          bool add_$1($0 val);
          void set_$1(size_t index, $0 val);
          bool resize_$1(size_t len);
        )cc",
        CppConstType(field), resolved_field_name);
  }
}

void WriteRepeatedMessageAccessor(const protobuf::Descriptor* desc,
                                  const protobuf::FieldDescriptor* field,
                                  const absl::string_view resolved_field_name,
                                  const absl::string_view class_name,
                                  Context& ctx) {
  const char arena_expression[] = "arena_";
  absl::string_view upbc_name = field->name();
  ctx.EmitLegacy(
      R"cc(
        $1 $0::$2(size_t index) const {
          size_t len;
          auto* ptr = $3_$5(msg_, &len);
          assert(index < len);
          return ::hpb::interop::upb::MakeCHandle<$4>(
              (upb_Message*)*(ptr + index), arena_);
        }
      )cc",
      class_name, MessagePtrConstType(field, /* is_const */ true),
      resolved_field_name, upb::generator::CApiMessageType(desc->full_name()),
      MessageBaseType(field, /* maybe_const */ false), upbc_name);
  ctx.EmitLegacy(
      R"cc(
        absl::StatusOr<$1> $0::add_$2() {
          auto new_msg = $3_add_$6(msg_, $5);
          if (!new_msg) {
            return ::hpb::MessageAllocationError();
          }
          return hpb::interop::upb::MakeHandle<$4>((upb_Message *)new_msg, $5);
        }

        bool $0::add_alias_$2($1 target) {
          ABSL_CHECK(upb_Arena_IsFused(arena_, hpb::interop::upb::GetArena(target)));
          size_t size = 0;
          $3_$2(msg_, &size);
          auto elements = $3_resize_$2(msg_, size + 1, arena_);
          if (!elements) {
            return false;
          }
          elements[size] = ($9 *)hpb::interop::upb::GetMessage(target);
          return true;
        }
      )cc",
      class_name, MessagePtrConstType(field, /* const */ false),
      resolved_field_name, upb::generator::CApiMessageType(desc->full_name()),
      MessageBaseType(field, /* maybe_const */ false), arena_expression,
      upbc_name, ClassName(desc), field->index(),
      upb::generator::CApiMessageType(field->message_type()->full_name()));
  ctx.EmitLegacy(
      R"cc(
        $1 $0::mutable_$2(size_t index) const {
          size_t len;
          auto* ptr = $3_$6(msg_, &len);
          assert(index < len);
          return hpb::interop::upb::MakeHandle<$4>((upb_Message*)*(ptr + index), $5);
        }
      )cc",
      class_name, MessagePtrConstType(field, /* is_const */ false),
      resolved_field_name, upb::generator::CApiMessageType(desc->full_name()),
      MessageBaseType(field, /* maybe_const */ false), arena_expression,
      upbc_name);
  ctx.EmitLegacy(
      R"cc(
        const ::hpb::RepeatedField<const $1>::CProxy $0::$2() const {
          size_t size;
          const upb_Array* arr = _$3_$4_$5(msg_, &size);
          return ::hpb::RepeatedField<const $1>::CProxy(arr, arena_);
        };
        ::hpb::Ptr<::hpb::RepeatedField<$1>> $0::mutable_$2() {
          size_t size;
          upb_Array* arr = _$3_$4_$6(msg_, &size, arena_);
          return ::hpb::RepeatedField<$1>::Proxy(arr, arena_);
        }
      )cc",
      class_name,                                              // $0
      MessageBaseType(field, /* maybe_const */ false),         // $1
      resolved_field_name,                                     // $2
      upb::generator::CApiMessageType(desc->full_name()),      // $3
      upbc_name,                                               // $4
      upb::generator::kRepeatedFieldArrayGetterPostfix,        // $5
      upb::generator::kRepeatedFieldMutableArrayGetterPostfix  // $6
  );
}

void WriteRepeatedStringAccessor(const protobuf::Descriptor* desc,
                                 const protobuf::FieldDescriptor* field,
                                 const absl::string_view resolved_field_name,
                                 const absl::string_view class_name,
                                 Context& ctx) {
  absl::string_view upbc_name = field->name();
  ctx.EmitLegacy(
      R"cc(
        $1 $0::$2(size_t index) const {
          size_t len;
          auto* ptr = $3_mutable_$4(msg_, &len);
          assert(index < len);
          return hpb::interop::upb::FromUpbStringView(*(ptr + index));
        }
      )cc",
      class_name, CppConstType(field), resolved_field_name,
      upb::generator::CApiMessageType(desc->full_name()), upbc_name);
  ctx.EmitLegacy(
      R"cc(
        bool $0::resize_$1(size_t len) {
          return $2_resize_$3(msg_, len, arena_);
        }
      )cc",
      class_name, resolved_field_name,
      upb::generator::CApiMessageType(desc->full_name()), upbc_name);
  ctx.EmitLegacy(
      R"cc(
        bool $0::add_$2($1 val) {
          return $3_add_$4(msg_,
                           hpb::interop::upb::CopyToUpbStringView(val, arena_),
                           arena_);
        }
      )cc",
      class_name, CppConstType(field), resolved_field_name,
      upb::generator::CApiMessageType(desc->full_name()), upbc_name);
  ctx.EmitLegacy(
      R"cc(
        void $0::set_$2(size_t index, $1 val) {
          size_t len;
          auto* ptr = $3_mutable_$4(msg_, &len);
          assert(index < len);
          *(ptr + index) = hpb::interop::upb::CopyToUpbStringView(val, arena_);
        }
      )cc",
      class_name, CppConstType(field), resolved_field_name,
      upb::generator::CApiMessageType(desc->full_name()), upbc_name);
  ctx.EmitLegacy(
      R"cc(
        const ::hpb::RepeatedField<$1>::CProxy $0::$2() const {
          size_t size;
          const upb_Array* arr = _$3_$4_$5(msg_, &size);
          return ::hpb::RepeatedField<$1>::CProxy(arr, arena_);
        };
        ::hpb::Ptr<::hpb::RepeatedField<$1>> $0::mutable_$2() {
          size_t size;
          upb_Array* arr = _$3_$4_$6(msg_, &size, arena_);
          return ::hpb::RepeatedField<$1>::Proxy(arr, arena_);
        }
      )cc",
      class_name,                                              // $0
      CppConstType(field),                                     // $1
      resolved_field_name,                                     // $2
      upb::generator::CApiMessageType(desc->full_name()),      // $3
      upbc_name,                                               // $4
      upb::generator::kRepeatedFieldArrayGetterPostfix,        // $5
      upb::generator::kRepeatedFieldMutableArrayGetterPostfix  // $6
  );
}

void WriteRepeatedScalarAccessor(const protobuf::Descriptor* desc,
                                 const protobuf::FieldDescriptor* field,
                                 const absl::string_view resolved_field_name,
                                 const absl::string_view class_name,
                                 Context& ctx) {
  absl::string_view upbc_name = field->name();
  ctx.EmitLegacy(
      R"cc(
        $1 $0::$2(size_t index) const {
          size_t len;
          auto* ptr = $3_mutable_$4(msg_, &len);
          assert(index < len);
          return *(ptr + index);
        }
      )cc",
      class_name, CppConstType(field), resolved_field_name,
      upb::generator::CApiMessageType(desc->full_name()), upbc_name);
  ctx.EmitLegacy(
      R"cc(
        bool $0::resize_$1(size_t len) {
          return $2_resize_$3(msg_, len, arena_);
        }
      )cc",
      class_name, resolved_field_name,
      upb::generator::CApiMessageType(desc->full_name()), upbc_name);
  ctx.EmitLegacy(
      R"cc(
        bool $0::add_$2($1 val) { return $3_add_$4(msg_, val, arena_); }
      )cc",
      class_name, CppConstType(field), resolved_field_name,
      upb::generator::CApiMessageType(desc->full_name()), upbc_name);
  ctx.EmitLegacy(
      R"cc(
        void $0::set_$2(size_t index, $1 val) {
          size_t len;
          auto* ptr = $3_mutable_$4(msg_, &len);
          assert(index < len);
          *(ptr + index) = val;
        }
      )cc",
      class_name, CppConstType(field), resolved_field_name,
      upb::generator::CApiMessageType(desc->full_name()), upbc_name);
  ctx.EmitLegacy(
      R"cc(
        const ::hpb::RepeatedField<$1>::CProxy $0::$2() const {
          size_t size;
          const upb_Array* arr = _$3_$4_$5(msg_, &size);
          return ::hpb::RepeatedField<$1>::CProxy(arr, arena_);
        };
        ::hpb::Ptr<::hpb::RepeatedField<$1>> $0::mutable_$2() {
          size_t size;
          upb_Array* arr = _$3_$4_$6(msg_, &size, arena_);
          return ::hpb::RepeatedField<$1>::Proxy(arr, arena_);
        }
      )cc",
      class_name,                                              // $0
      CppConstType(field),                                     // $1
      resolved_field_name,                                     // $2
      upb::generator::CApiMessageType(desc->full_name()),      // $3
      upbc_name,                                               // $4
      upb::generator::kRepeatedFieldArrayGetterPostfix,        // $5
      upb::generator::kRepeatedFieldMutableArrayGetterPostfix  // $6
  );
}

}  // namespace protobuf
}  // namespace google::hpb_generator
