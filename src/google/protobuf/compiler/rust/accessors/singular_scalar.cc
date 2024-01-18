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
#include "google/protobuf/compiler/rust/accessors/accessor_generator.h"
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
          {"view_self", ViewReceiver(accessor_case)},
          {"Scalar", RsTypePath(ctx, field)},
          {"hazzer_thunk", ThunkName(ctx, field, "has")},
          {"default_value", DefaultValue(ctx, field)},
          {"getter",
           [&] {
             ctx.Emit({}, R"rs(
                  pub fn $field$($view_self$) -> $Scalar$ {
                    unsafe { $getter_thunk$(self.raw_msg()) }
                  }
                )rs");
           }},
          {"getter_opt",
           [&] {
             if (!field.is_optional()) return;
             if (!field.has_presence()) return;
             ctx.Emit({}, R"rs(
                  pub fn $field$_opt($view_self$) -> $pb$::Optional<$Scalar$> {
                    if !unsafe { $hazzer_thunk$(self.raw_msg()) } {
                      return $pb$::Optional::Unset($default_value$);
                    }
                    let value = unsafe { $getter_thunk$(self.raw_msg()) };
                    $pb$::Optional::Set(value)
                  }
                  )rs");
           }},
          {"getter_thunk", ThunkName(ctx, field, "get")},
          {"setter_thunk", ThunkName(ctx, field, "set")},
          {"clearer_thunk", ThunkName(ctx, field, "clear")},
          {"getter_mut",
           [&] {
             if (accessor_case == AccessorCase::VIEW) {
               return;
             }
             if (field.has_presence()) {
               ctx.Emit({}, R"rs(
                  pub fn $field$_mut(&mut self) -> $pb$::FieldEntry<'_, $Scalar$> {
                    static VTABLE: $pbi$::PrimitiveOptionalMutVTable<$Scalar$> =
                      $pbi$::PrimitiveOptionalMutVTable::new(
                        $pbi$::Private,
                        $getter_thunk$,
                        $setter_thunk$,
                        $clearer_thunk$,
                        $default_value$,
                      );

                      unsafe {
                        let has = $hazzer_thunk$(self.raw_msg());
                        $pbi$::new_vtable_field_entry::<$Scalar$>(
                          $pbi$::Private,
                          self.as_mutator_message_ref(),
                          &VTABLE,
                          has,
                        )
                      }
                  }
                )rs");
             } else {
               ctx.Emit({}, R"rs(
                  pub fn $field$_mut(&mut self) -> $pb$::Mut<'_, $Scalar$> {
                    static VTABLE: $pbi$::PrimitiveVTable<$Scalar$> =
                      $pbi$::PrimitiveVTable::new(
                        $pbi$::Private,
                        $getter_thunk$,
                        $setter_thunk$,
                      );

                      // SAFETY:
                      // - The message is valid for the output lifetime.
                      // - The vtable is valid for the field.
                      // - There is no way to mutate the element for the output
                      //   lifetime except through this mutator.
                      unsafe {
                        $pb$::PrimitiveMut::from_inner(
                          $pbi$::Private,
                          $pbi$::RawVTableMutator::new(
                            $pbi$::Private,
                            self.as_mutator_message_ref(),
                            &VTABLE,
                          ),
                        )
                      }
                  }
                )rs");
             }
           }},
      },
      R"rs(
          $getter$
          $getter_opt$
          $getter_mut$
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
                 ctx.Emit(
                     R"rs(
                  fn $hazzer_thunk$(raw_msg: $pbi$::RawMessage) -> bool;
                  fn $clearer_thunk$(raw_msg: $pbi$::RawMessage);
                )rs");
               }
             }}},
           R"rs(
          $with_presence_fields_thunks$
          fn $getter_thunk$(raw_msg: $pbi$::RawMessage) -> $Scalar$;
          fn $setter_thunk$(raw_msg: $pbi$::RawMessage, val: $Scalar$);
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
               if (field.has_presence()) {
                 ctx.Emit(R"cc(
                   bool $hazzer_thunk$($QualifiedMsg$* msg) {
                     return msg->has_$field$();
                   }
                   void $clearer_thunk$($QualifiedMsg$* msg) { msg->clear_$field$(); }
                 )cc");
               }
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
