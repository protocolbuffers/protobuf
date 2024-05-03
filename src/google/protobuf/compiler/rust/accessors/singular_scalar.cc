// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include <string>

#include "absl/strings/string_view.h"
#include "google/protobuf/compiler/cpp/helpers.h"
#include "google/protobuf/compiler/rust/accessors/accessor_case.h"
#include "google/protobuf/compiler/rust/accessors/generator.h"
#include "google/protobuf/compiler/rust/accessors/helpers.h"
#include "google/protobuf/compiler/rust/context.h"
#include "google/protobuf/compiler/rust/naming.h"
#include "google/protobuf/descriptor.h"

namespace google {
namespace protobuf {
namespace compiler {
namespace rust {

void SingularScalar::InMsgImpl(Context& ctx, const FieldDescriptor& field,
                               AccessorCase accessor_case) const {
  ctx.Emit(
      {
          {"field", RsSafeName(field.name())},
          {"raw_field_name", field.name()},  // Never r# prefixed
          {"view_self", ViewReceiver(accessor_case)},
          {"Scalar", RsTypePath(ctx, field)},
          {"hazzer_thunk", ThunkName(ctx, field, "has")},
          {"default_value", DefaultValue(ctx, field)},
          {"getter",
           [&] {
             ctx.Emit(R"rs(
                  pub fn $field$($view_self$) -> $Scalar$ {
                    unsafe { $getter_thunk$(self.raw_msg()) }
                  }
                )rs");
           }},
          {"getter_opt",
           [&] {
             if (!field.has_presence()) return;
             ctx.Emit(R"rs(
                  pub fn $raw_field_name$_opt($view_self$) -> $pb$::Optional<$Scalar$> {
                    if self.has_$raw_field_name$() {
                      $pb$::Optional::Set(self.$field$())
                    } else {
                      $pb$::Optional::Unset($default_value$)
                    }
                  }
                  )rs");
           }},
          {"setter",
           [&] {
             if (accessor_case == AccessorCase::VIEW) return;
             ctx.Emit({}, R"rs(
                 pub fn set_$raw_field_name$(&mut self, val: $Scalar$) {
                   unsafe { $setter_thunk$(self.raw_msg(), val) }
                 }
               )rs");
           }},
          {"hazzer",
           [&] {
             if (!field.has_presence()) return;
             ctx.Emit({}, R"rs(
                pub fn has_$raw_field_name$($view_self$) -> bool {
                  unsafe { $hazzer_thunk$(self.raw_msg()) }
                })rs");
           }},
          {"clearer",
           [&] {
             if (accessor_case == AccessorCase::VIEW) return;
             if (!field.has_presence()) return;
             ctx.Emit({}, R"rs(
                  pub fn clear_$raw_field_name$(&mut self) {
                    unsafe { $clearer_thunk$(self.raw_msg()) }
                  })rs");
           }},
          {"getter_thunk", ThunkName(ctx, field, "get")},
          {"setter_thunk", ThunkName(ctx, field, "set")},
          {"clearer_thunk", ThunkName(ctx, field, "clear")},
      },
      R"rs(
          $getter$
          $getter_opt$
          $setter$
          $hazzer$
          $clearer$
        )rs");
}

void SingularScalar::InExternC(Context& ctx,
                               const FieldDescriptor& field) const {
  // In order to soundly pass a Rust type to C/C++ as a function argument,
  // the types must be FFI-compatible.
  // This requires special consideration for enums, which aren't trivial
  // primitive types. Rust protobuf enums are defined as `#[repr(transparent)]`
  // over `i32`, making them ABI-compatible with `int32_t`.
  // Upb defines enum thunks as taking `int32_t`, and so we can pass Rust enums
  // directly to thunks without any cast.
  ctx.Emit({{"Scalar", RsTypePath(ctx, field)},
            {"hazzer_thunk", ThunkName(ctx, field, "has")},
            {"getter_thunk", ThunkName(ctx, field, "get")},
            {"setter_thunk", ThunkName(ctx, field, "set")},
            {"clearer_thunk", ThunkName(ctx, field, "clear")},
            {"with_presence_fields_thunks",
             [&] {
               if (field.has_presence()) {
                 ctx.Emit(R"rs(
                  fn $hazzer_thunk$(raw_msg: $pbr$::RawMessage) -> bool;
                  fn $clearer_thunk$(raw_msg: $pbr$::RawMessage);
                )rs");
               }
             }}},
           R"rs(
          $with_presence_fields_thunks$
          fn $getter_thunk$(raw_msg: $pbr$::RawMessage) -> $Scalar$;
          fn $setter_thunk$(raw_msg: $pbr$::RawMessage, val: $Scalar$);
        )rs");
}

void SingularScalar::InThunkCc(Context& ctx,
                               const FieldDescriptor& field) const {
  std::string scalar;
  auto enum_ = field.enum_type();
  if (enum_ != nullptr) {
    // The C++ runtime defines its thunks as receiving enum types.
    // This is fine since:
    // - the C++ runtime represents enums as `int`
    // - the C++ runtime guarantees `int` is a `int32_t`.
    // - Rust gencode defines enums as `#[repr(transparent)]` over `i32`.
    scalar = cpp::QualifiedClassName(enum_);
  } else {
    scalar = cpp::PrimitiveTypeName(field.cpp_type());
  }

  ctx.Emit({{"field", cpp::FieldName(&field)},
            {"Scalar", scalar},
            {"QualifiedMsg", cpp::QualifiedClassName(field.containing_type())},
            {"hazzer_thunk", ThunkName(ctx, field, "has")},
            {"getter_thunk", ThunkName(ctx, field, "get")},
            {"setter_thunk", ThunkName(ctx, field, "set")},
            {"clearer_thunk", ThunkName(ctx, field, "clear")},
            {"with_presence_fields_thunks",
             [&] {
               if (!field.has_presence()) return;
               ctx.Emit(R"cc(
                 bool $hazzer_thunk$($QualifiedMsg$* msg) {
                   return msg->has_$field$();
                 }
                 void $clearer_thunk$($QualifiedMsg$* msg) { msg->clear_$field$(); }
               )cc");
             }}},
           R"cc(
             $with_presence_fields_thunks$;
             $Scalar$ $getter_thunk$($QualifiedMsg$* msg) { return msg->$field$(); }
             void $setter_thunk$($QualifiedMsg$* msg, $Scalar$ val) {
               msg->set_$field$(val);
             }
           )cc");
}

}  // namespace rust
}  // namespace compiler
}  // namespace protobuf
}  // namespace google
