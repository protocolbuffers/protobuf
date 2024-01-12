// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "absl/strings/string_view.h"
#include "google/protobuf/compiler/cpp/helpers.h"
#include "google/protobuf/compiler/rust/accessors/accessor_generator.h"
#include "google/protobuf/compiler/rust/context.h"
#include "google/protobuf/compiler/rust/naming.h"
#include "google/protobuf/descriptor.h"

namespace google {
namespace protobuf {
namespace compiler {
namespace rust {

void RepeatedScalar::InMsgImpl(Context& ctx,
                               const FieldDescriptor& field) const {
  ctx.Emit({{"field", field.name()},
            {"Scalar", RsTypePath(ctx, field)},
            {"getter_thunk", ThunkName(ctx, field, "get")},
            {"getter_mut_thunk", ThunkName(ctx, field, "get_mut")},
            {"getter",
             [&] {
               if (ctx.is_upb()) {
                 ctx.Emit({}, R"rs(
                    pub fn r#$field$(&self) -> $pb$::RepeatedView<'_, $Scalar$> {
                      unsafe {
                        $getter_thunk$(
                          self.inner.msg,
                          /* optional size pointer */ std::ptr::null(),
                        ) }
                        .map_or_else(
                          $pbr$::empty_array::<$Scalar$>,
                          |raw| unsafe {
                            $pb$::RepeatedView::from_raw($pbi$::Private, raw)
                          }
                        )
                    }
                  )rs");
               } else {
                 ctx.Emit({}, R"rs(
                    pub fn r#$field$(&self) -> $pb$::RepeatedView<'_, $Scalar$> {
                      unsafe {
                        $pb$::RepeatedView::from_raw(
                          $pbi$::Private,
                          unsafe { $getter_thunk$(self.inner.msg) },
                        )
                      }
                    }
                  )rs");
               }
             }},
            {"clearer_thunk", ThunkName(ctx, field, "clear")},
            {"field_mutator_getter",
             [&] {
               if (ctx.is_upb()) {
                 ctx.Emit({}, R"rs(
                    pub fn r#$field$_mut(&mut self) -> $pb$::RepeatedMut<'_, $Scalar$> {
                      unsafe {
                        $pb$::RepeatedMut::from_inner(
                          $pbi$::Private,
                          $pbr$::InnerRepeatedMut::new(
                            $pbi$::Private,
                            $getter_mut_thunk$(
                              self.inner.msg,
                              /* optional size pointer */ std::ptr::null(),
                              self.inner.arena.raw(),
                            ),
                            &self.inner.arena,
                          ),
                        )
                      }
                    }
                  )rs");
               } else {
                 ctx.Emit({}, R"rs(
                      pub fn r#$field$_mut(&mut self) -> $pb$::RepeatedMut<'_, $Scalar$> {
                        unsafe {
                          $pb$::RepeatedMut::from_inner(
                            $pbi$::Private,
                            $pbr$::InnerRepeatedMut::new(
                              $pbi$::Private,
                              $getter_mut_thunk$(self.inner.msg),
                            ),
                          )
                        }
                      }
                    )rs");
               }
             }}},
           R"rs(
          $getter$
          $field_mutator_getter$
        )rs");
}

void RepeatedScalar::InExternC(Context& ctx,
                               const FieldDescriptor& field) const {
  ctx.Emit({{"Scalar", RsTypePath(ctx, field)},
            {"getter_thunk", ThunkName(ctx, field, "get")},
            {"getter_mut_thunk", ThunkName(ctx, field, "get_mut")},
            {"getter",
             [&] {
               if (ctx.is_upb()) {
                 ctx.Emit(R"rs(
                    fn $getter_mut_thunk$(
                      raw_msg: $pbi$::RawMessage,
                      size: *const usize,
                      arena: $pbi$::RawArena,
                    ) -> $pbi$::RawRepeatedField;
                    //  Returns `None` when returned array pointer is NULL.
                    fn $getter_thunk$(
                      raw_msg: $pbi$::RawMessage,
                      size: *const usize,
                    ) -> Option<$pbi$::RawRepeatedField>;
                  )rs");
               } else {
                 ctx.Emit(R"rs(
                    fn $getter_mut_thunk$(raw_msg: $pbi$::RawMessage) -> $pbi$::RawRepeatedField;
                    fn $getter_thunk$(raw_msg: $pbi$::RawMessage) -> $pbi$::RawRepeatedField;
                  )rs");
               }
             }},
            {"clearer_thunk", ThunkName(ctx, field, "clear")}},
           R"rs(
          fn $clearer_thunk$(raw_msg: $pbi$::RawMessage);
          $getter$
        )rs");
}

void RepeatedScalar::InThunkCc(Context& ctx,
                               const FieldDescriptor& field) const {
  ctx.Emit({{"field", cpp::FieldName(&field)},
            {"Scalar", cpp::PrimitiveTypeName(field.cpp_type())},
            {"QualifiedMsg", cpp::QualifiedClassName(field.containing_type())},
            {"clearer_thunk", ThunkName(ctx, field, "clear")},
            {"getter_thunk", ThunkName(ctx, field, "get")},
            {"getter_mut_thunk", ThunkName(ctx, field, "get_mut")},
            {"impls",
             [&] {
               ctx.Emit(
                   R"cc(
                     void $clearer_thunk$($QualifiedMsg$* msg) {
                       msg->clear_$field$();
                     }
                     google::protobuf::RepeatedField<$Scalar$>* $getter_mut_thunk$($QualifiedMsg$* msg) {
                       return msg->mutable_$field$();
                     }
                     const google::protobuf::RepeatedField<$Scalar$>* $getter_thunk$(
                         const $QualifiedMsg$* msg) {
                       return &msg->$field$();
                     }
                   )cc");
             }}},
           "$impls$");
}

}  // namespace rust
}  // namespace compiler
}  // namespace protobuf
}  // namespace google
