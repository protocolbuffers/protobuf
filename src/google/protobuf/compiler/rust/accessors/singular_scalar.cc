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
#include "google/protobuf/compiler/rust/accessors/with_presence.h"
#include "google/protobuf/compiler/rust/context.h"
#include "google/protobuf/compiler/rust/naming.h"
#include "google/protobuf/compiler/rust/upb_helpers.h"
#include "google/protobuf/descriptor.h"

namespace google {
namespace protobuf {
namespace compiler {
namespace rust {

namespace {

// The type name to use for the get/set methods on `MessagePtr`, e.g. `i32`
// for `MessagePtr::get_i32_at_index()` and
// `MessagePtr::set_base_field_i32_at_index()`.
std::string UpbCTypeNameForFunctions(const FieldDescriptor& field) {
  switch (field.cpp_type()) {
    case FieldDescriptor::CPPTYPE_INT32:
      return "i32";
    case FieldDescriptor::CPPTYPE_INT64:
      return "i64";
    case FieldDescriptor::CPPTYPE_UINT32:
      return "u32";
    case FieldDescriptor::CPPTYPE_UINT64:
      return "u64";
    case FieldDescriptor::CPPTYPE_DOUBLE:
      return "f64";
    case FieldDescriptor::CPPTYPE_FLOAT:
      return "f32";
    case FieldDescriptor::CPPTYPE_BOOL:
      return "bool";
    case FieldDescriptor::CPPTYPE_ENUM:
      return "i32";
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
  if (field.has_presence()) {
    WithPresenceAccessorsInMsgImpl(ctx, field, accessor_case);
  }

  std::string field_name = FieldNameWithCollisionAvoidance(field);

  ctx.Emit(
      {
          {"field", RsSafeName(field_name)},
          {"raw_field_name", field_name},  // Never r# prefixed
          {"view_self", ViewReceiver(accessor_case)},
          {"Scalar", RsTypePath(ctx, field)},
          {"default_value", DefaultValue(ctx, field)},
          {"upb_mt_field_index", UpbMiniTableFieldIndex(field)},
          {"type_name", UpbCTypeNameForFunctions(field)},
          {"getter",
           [&] {
             if (ctx.is_cpp()) {
               ctx.Emit({{"getter_thunk", ThunkName(ctx, field, "get")}}, R"rs(
                    pub fn $field$($view_self$) -> $Scalar$ {
                      unsafe { $getter_thunk$(self.raw_msg()) }
                    }
                  )rs");
             } else {
               ctx.Emit(
                   R"rs(
                    pub fn $field$($view_self$) -> $Scalar$ {
                      unsafe {
                        // TODO: b/361751487: This .into() and .try_into() is only
                        // here for the enum<->i32 case, we should avoid it for
                        // other primitives where the types naturally match
                        // perfectly (and do an unchecked conversion for
                        // i32->enum types, since even for closed enums we trust
                        // upb to only return one of the named values).
                        self.inner.ptr().get_$type_name$_at_index(
                          $upb_mt_field_index$, ($default_value$).into()
                        ).try_into().unwrap()
                      }
                    }
                  )rs");
             }
           }},
          {"setter",
           [&] {
             if (accessor_case == AccessorCase::VIEW) return;
             if (ctx.is_cpp()) {
               ctx.Emit({{"setter_thunk", ThunkName(ctx, field, "set")}}, R"rs(
                  pub fn set_$raw_field_name$(&mut self, val: $Scalar$) {
                    unsafe { $setter_thunk$(self.raw_msg(), val) }
                  }
                )rs");
             } else {
               ctx.Emit(R"rs(
                  pub fn set_$raw_field_name$(&mut self, val: $Scalar$) {
                    unsafe {
                      // TODO: b/361751487: This .into() is only here
                      // here for the enum<->i32 case, we should avoid it for
                      // other primitives where the types naturally match
                      //perfectly.
                      self.inner.ptr_mut().set_base_field_$type_name$_at_index(
                        $upb_mt_field_index$, val.into()
                      )
                    }
                  }
                )rs");
             }
           }},
      },
      R"rs(
          $getter$
          $setter$
        )rs");
}

void SingularScalar::InExternC(Context& ctx,
                               const FieldDescriptor& field) const {
  ABSL_CHECK(ctx.is_cpp());

  if (field.has_presence()) {
    WithPresenceAccessorsInExternC(ctx, field);
  }

  // In order to soundly pass a Rust type to C/C++ as a function argument,
  // the types must be FFI-compatible.
  // This requires special consideration for enums, which aren't trivial
  // primitive types. Rust protobuf enums are defined as `#[repr(transparent)]`
  // over `i32`, making them ABI-compatible with `int32_t`.
  // Upb defines enum thunks as taking `int32_t`, and so we can pass Rust enums
  // directly to thunks without any cast.
  ctx.Emit({{"Scalar", RsTypePath(ctx, field)},
            {"getter_thunk", ThunkName(ctx, field, "get")},
            {"setter_thunk", ThunkName(ctx, field, "set")}},
           R"rs(
          fn $getter_thunk$(raw_msg: $pbr$::RawMessage) -> $Scalar$;
          fn $setter_thunk$(raw_msg: $pbr$::RawMessage, val: $Scalar$);
        )rs");
}

void SingularScalar::InThunkCc(Context& ctx,
                               const FieldDescriptor& field) const {
  ABSL_CHECK(ctx.is_cpp());

  if (field.has_presence()) {
    WithPresenceAccessorsInThunkCc(ctx, field);
  }

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
            {"getter_thunk", ThunkName(ctx, field, "get")},
            {"setter_thunk", ThunkName(ctx, field, "set")}},
           R"cc(
             $Scalar$ $getter_thunk$(const $QualifiedMsg$* msg) {
               return msg->$field$();
             }
             void $setter_thunk$($QualifiedMsg$* msg, $Scalar$ val) {
               msg->set_$field$(val);
             }
           )cc");
}

}  // namespace rust
}  // namespace compiler
}  // namespace protobuf
}  // namespace google
