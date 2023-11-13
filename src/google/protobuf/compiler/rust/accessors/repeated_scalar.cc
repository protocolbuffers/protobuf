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

void RepeatedScalar::InMsgImpl(Context<FieldDescriptor> field) const {
  field.Emit({{"field", field.desc().name()},
              {"Scalar", PrimitiveRsTypeName(field.desc())},
              {"getter_thunk", Thunk(field, "get")},
              {"getter_mut_thunk", Thunk(field, "get_mut")},
              {"getter",
               [&] {
                 if (field.is_upb()) {
                   field.Emit({}, R"rs(
                    pub fn r#$field$(&self) -> $pb$::RepeatedView<'_, $Scalar$> {
                      let inner = unsafe {
                        $getter_thunk$(
                          self.inner.msg,
                          /* optional size pointer */ std::ptr::null(),
                        ) }
                        .map_or_else(|| unsafe {$pbr$::empty_array()}, |raw| {
                          $pbr$::RepeatedFieldInner{ raw, arena: &self.inner.arena }
                        });
                      $pb$::RepeatedView::from_inner($pbi$::Private, inner)
                    }
                  )rs");
                 } else {
                   field.Emit({}, R"rs(
                    pub fn r#$field$(&self) -> $pb$::RepeatedView<'_, $Scalar$> {
                      $pb$::RepeatedView::from_inner(
                        $pbi$::Private,
                        $pbr$::RepeatedFieldInner{
                          raw: unsafe { $getter_thunk$(self.inner.msg) },
                          _phantom: std::marker::PhantomData,
                        },
                      )
                    }
                  )rs");
                 }
               }},
              {"clearer_thunk", Thunk(field, "clear")},
              {"field_mutator_getter",
               [&] {
                 if (field.is_upb()) {
                   field.Emit({}, R"rs(
                    pub fn r#$field$_mut(&mut self) -> $pb$::RepeatedMut<'_, $Scalar$> {
                      $pb$::RepeatedMut::from_inner(
                        $pbi$::Private,
                        $pbr$::RepeatedFieldInner{
                          raw: unsafe { $getter_mut_thunk$(
                            self.inner.msg,
                            /* optional size pointer */ std::ptr::null(),
                            self.inner.arena.raw(),
                          ) },
                          arena: &self.inner.arena,
                        },
                      )
                    }
                  )rs");
                 } else {
                   field.Emit({}, R"rs(
                      pub fn r#$field$_mut(&mut self) -> $pb$::RepeatedMut<'_, $Scalar$> {
                        $pb$::RepeatedMut::from_inner(
                          $pbi$::Private,
                          $pbr$::RepeatedFieldInner{
                            raw: unsafe { $getter_mut_thunk$(self.inner.msg)},
                            _phantom: std::marker::PhantomData,
                          },
                        )
                      }
                    )rs");
                 }
               }}},
             R"rs(
          $getter$
          $field_mutator_getter$
        )rs");
}

void RepeatedScalar::InExternC(Context<FieldDescriptor> field) const {
  field.Emit({{"Scalar", PrimitiveRsTypeName(field.desc())},
              {"getter_thunk", Thunk(field, "get")},
              {"getter_mut_thunk", Thunk(field, "get_mut")},
              {"getter",
               [&] {
                 if (field.is_upb()) {
                   field.Emit(R"rs(
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
                   field.Emit(R"rs(
                    fn $getter_mut_thunk$(raw_msg: $pbi$::RawMessage) -> $pbi$::RawRepeatedField;
                    fn $getter_thunk$(raw_msg: $pbi$::RawMessage) -> $pbi$::RawRepeatedField;
                  )rs");
                 }
               }},
              {"clearer_thunk", Thunk(field, "clear")}},
             R"rs(
          fn $clearer_thunk$(raw_msg: $pbi$::RawMessage);
          $getter$
        )rs");
}

void RepeatedScalar::InThunkCc(Context<FieldDescriptor> field) const {
  field.Emit({{"field", cpp::FieldName(&field.desc())},
              {"Scalar", cpp::PrimitiveTypeName(field.desc().cpp_type())},
              {"QualifiedMsg",
               cpp::QualifiedClassName(field.desc().containing_type())},
              {"clearer_thunk", Thunk(field, "clear")},
              {"getter_thunk", Thunk(field, "get")},
              {"getter_mut_thunk", Thunk(field, "get_mut")},
              {"impls",
               [&] {
                 field.Emit(
                     R"cc(
                       void $clearer_thunk$($QualifiedMsg$* msg) {
                         msg->clear_$field$();
                       }
                       google::protobuf::RepeatedField<$Scalar$>* $getter_mut_thunk$($QualifiedMsg$* msg) {
                         return msg->mutable_$field$();
                       }
                       const google::protobuf::RepeatedField<$Scalar$>& $getter_thunk$($QualifiedMsg$& msg) {
                         return msg.$field$();
                       }
                     )cc");
               }}},
             "$impls$");
}

}  // namespace rust
}  // namespace compiler
}  // namespace protobuf
}  // namespace google
