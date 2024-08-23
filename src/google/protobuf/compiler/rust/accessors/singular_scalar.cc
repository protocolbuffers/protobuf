// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include <string>

#include "absl/log/absl_check.h"
#include "absl/strings/string_view.h"
#include "google/protobuf/compiler/cpp/helpers.h"
#include "google/protobuf/compiler/rust/accessors/accessor_case.h"
#include "google/protobuf/compiler/rust/accessors/default_value.h"
#include "google/protobuf/compiler/rust/accessors/generator.h"
#include "google/protobuf/compiler/rust/context.h"
#include "google/protobuf/compiler/rust/naming.h"
#include "google/protobuf/compiler/rust/upb_helpers.h"
#include "google/protobuf/descriptor.h"

namespace google {
namespace protobuf {
namespace compiler {
namespace rust {

namespace {

// The upb function to use for the get/set functions, eg `Int32` for the
// functions `upb_Message_GetInt32` and upb_Message_SetInt32`.
std::string UpbCTypeNameForFunctions(const FieldDescriptor& field) {
  switch (field.cpp_type()) {
    case FieldDescriptor::CPPTYPE_INT32:
      return "Int32";
    case FieldDescriptor::CPPTYPE_INT64:
      return "Int64";
    case FieldDescriptor::CPPTYPE_UINT32:
      return "UInt32";
    case FieldDescriptor::CPPTYPE_UINT64:
      return "UInt64";
    case FieldDescriptor::CPPTYPE_DOUBLE:
      return "Double";
    case FieldDescriptor::CPPTYPE_FLOAT:
      return "Float";
    case FieldDescriptor::CPPTYPE_BOOL:
      return "Bool";
    case FieldDescriptor::CPPTYPE_ENUM:
      return "Int32";
    case FieldDescriptor::CPPTYPE_STRING:
    case FieldDescriptor::CPPTYPE_MESSAGE:
      // Handled by a different file.
      break;
  }
  ABSL_CHECK(false) << "Unexpected field type: " << field.cpp_type_name();
  return "";
}

}  // namespace

void SingularScalar::InMsgImpl(Context& ctx, const FieldDescriptor& field,
                               AccessorCase accessor_case) const {
  std::string field_name = FieldNameWithCollisionAvoidance(field);

  ctx.Emit(
      {
          {"field", RsSafeName(field_name)},
          {"raw_field_name", field_name},  // Never r# prefixed
          {"view_self", ViewReceiver(accessor_case)},
          {"Scalar", RsTypePath(ctx, field)},
          {"hazzer_thunk", ThunkName(ctx, field, "has")},
          {"default_value", DefaultValue(ctx, field)},
          {"upb_mt_field_index", UpbMiniTableFieldIndex(field)},
          {"upb_fn_type_name", UpbCTypeNameForFunctions(field)},
          {"getter",
           [&] {
             if (ctx.is_cpp()) {
               ctx.Emit(R"rs(
                    pub fn $field$($view_self$) -> $Scalar$ {
                      unsafe { $getter_thunk$(self.raw_msg()) }
                    }
                  )rs");
             } else {
               ctx.Emit(
                   R"rs(
                    pub fn $field$($view_self$) -> $Scalar$ {
                      unsafe {
                        let mt = <Self as $pbr$::AssociatedMiniTable>::mini_table();
                        let f = $pbr$::upb_MiniTable_GetFieldByIndex(
                            mt, $upb_mt_field_index$);

                        // TODO: b/361751487: This .into() and .try_into() is only
                        // here for the enum<->i32 case, we should avoid it for
                        // other primitives where the types naturally match
                        // perfectly (and do an unchecked conversion for
                        // i32->enum types, since even for closed enums we trust
                        // upb to only return one of the named values).
                        $pbr$::upb_Message_Get$upb_fn_type_name$(
                            self.raw_msg(), f, ($default_value$).into()).try_into().unwrap()
                      }
                    }
                  )rs");
             }
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
             if (ctx.is_cpp()) {
               ctx.Emit(R"rs(
                  pub fn set_$raw_field_name$(&mut self, val: $Scalar$) {
                    unsafe { $setter_thunk$(self.raw_msg(), val) }
                  }
                )rs");
             } else {
               ctx.Emit(R"rs(
                  pub fn set_$raw_field_name$(&mut self, val: $Scalar$) {
                    unsafe {
                      let mt = <Self as $pbr$::AssociatedMiniTable>::mini_table();
                      let f = $pbr$::upb_MiniTable_GetFieldByIndex(
                          mt, $upb_mt_field_index$);
                      // TODO: b/361751487: This .into() is only here
                      // here for the enum<->i32 case, we should avoid it for
                      // other primitives where the types naturally match
                      // perfectly.
                      $pbr$::upb_Message_SetBaseField$upb_fn_type_name$(
                          self.raw_msg(), f, val.into());
                    }
                  }
                )rs");
             }
           }},
          {"hazzer",
           [&] {
             if (!field.has_presence()) return;
             if (ctx.is_cpp()) {
               ctx.Emit(R"rs(
                  pub fn has_$raw_field_name$($view_self$) -> bool {
                    unsafe { $hazzer_thunk$(self.raw_msg()) }
                  })rs");
             } else {
               ctx.Emit(R"rs(
                  pub fn has_$raw_field_name$($view_self$) -> bool {
                    unsafe {
                      let mt = <Self as $pbr$::AssociatedMiniTable>::mini_table();
                      let f = $pbr$::upb_MiniTable_GetFieldByIndex(
                          mt, $upb_mt_field_index$);
                      $pbr$::upb_Message_HasBaseField(self.raw_msg(), f)
                    }
                  })rs");
             }
           }},
          {"clearer",
           [&] {
             if (accessor_case == AccessorCase::VIEW) return;
             if (!field.has_presence()) return;
             if (ctx.is_cpp()) {
               ctx.Emit(R"rs(
                    pub fn clear_$raw_field_name$(&mut self) {
                      unsafe { $clearer_thunk$(self.raw_msg()) }
                    })rs");
             } else {
               ctx.Emit(R"rs(
                    pub fn clear_$raw_field_name$(&mut self) {
                      unsafe {
                        let mt = <Self as $pbr$::AssociatedMiniTable>::mini_table();
                        let f = $pbr$::upb_MiniTable_GetFieldByIndex(
                            mt, $upb_mt_field_index$);
                        $pbr$::upb_Message_ClearBaseField(self.raw_msg(), f);
                      }
                    })rs");
             }
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
  // Only cpp kernel uses thunks.
  if (ctx.is_upb()) return;

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
