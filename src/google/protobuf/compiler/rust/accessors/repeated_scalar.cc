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

/**
 * Similar to other generated fields, `RepeatedScalar` generates cpp thunks that
 * match the name of the analgous upb functions (hence the _mutable_upb_array
 * suffix, which actually returns a `RepeatedField<T>*` in cpp).
 *
 * This is similar to how `Message` is implemented, where each runtime
 * (cpp.rs/upb.rs) exposes a wrapper struct by the same name, but with different
 * members and the appropriate Send/Sync-ness.
 *
 */

void RepeatedScalar::InMsgImpl(Context<FieldDescriptor> field) const {
  field.Emit(
      {
          {"field", field.desc().name()},
          {"Scalar", PrimitiveRsTypeName(field.desc())},
          {"hazzer_thunk", Thunk(field, "has")},
          {"mutable_upb_array_thunk", Thunk(field, "mutable_upb_array")},
          {"getter",
           [&] {
             if (field.is_upb()) {
               field.Emit({}, R"rs(
                    pub fn r#$field$(&self) -> $pb$::RepeatedView<'_, $Scalar$> {
                      $pb$::RepeatedView::from_inner($pbr$::RepeatedFieldInner{ raw: unsafe { _$mutable_upb_array_thunk$(self.inner.msg, std::ptr::null(), self.inner.arena.raw())}, arena: &self.inner.arena} )
                    }
                  )rs");
             } else {
               field.Emit({}, R"rs(
                    pub fn r#$field$(&self) -> $pb$::RepeatedView<'_, $Scalar$> {
                      $pb$::RepeatedView::from_inner($pbr$::RepeatedFieldInner{ raw: unsafe { _$mutable_upb_array_thunk$(self.inner.msg)}, _phantom: std::marker::PhantomData} )
                    }
                  )rs");
             }
           }},
          {"getter_opt",
           [&] {
             if (!field.desc().is_optional()) return;
             if (!field.desc().has_presence()) return;
             field.Emit({}, R"rs(
                  pub fn r#$field$_opt(&self) -> $pb$::Optional<$Scalar$> {
                    if !unsafe { $hazzer_thunk$(self.inner.msg) } {
                      return $pb$::Optional::Unset(<$Scalar$>::default());
                    }
                    let value = unsafe { $getter_thunk$(self.inner.msg) };
                    $pb$::Optional::Set(value)
                  }
                  )rs");
           }},
          {"clearer_thunk", Thunk(field, "clear")},
          {"field_setter",
           [&] {
             if (field.desc().has_presence()) {
               field.Emit({}, R"rs(
                  pub fn r#$field$_set(&mut self, val: Option<$Scalar$>) {
                    match val {
                      Some(val) => unsafe { $setter_thunk$(self.inner.msg, val) },
                      None => unsafe { $clearer_thunk$(self.inner.msg) },
                    }
                  }
                )rs");
             }
           }},
          {"field_mutator_getter",
           [&] {
             if (field.is_upb()) {
               field.Emit({}, R"rs(
                  pub fn r#$field$_mut(&mut self) -> $pb$::RepeatedMut<'_, $Scalar$> {
                      $pb$::RepeatedMut::from_inner($pbr$::RepeatedFieldInner{ raw: unsafe { _$mutable_upb_array_thunk$(self.inner.msg, std::ptr::null(), self.inner.arena.raw())}, arena: &self.inner.arena})
                  }
                )rs");
             } else {
               field.Emit({}, R"rs(
                  pub fn r#$field$_mut(&mut self) -> $pb$::RepeatedMut<'_, $Scalar$> {
                      $pb$::RepeatedMut::from_inner($pbr$::RepeatedFieldInner{ raw: unsafe { _$mutable_upb_array_thunk$(self.inner.msg)}, _phantom: std::marker::PhantomData})
                  }
                )rs");
             }
           }},
      },
      R"rs(
          $getter$
          $getter_opt$
          $field_setter$
          $field_mutator_getter$
        )rs");
}

void RepeatedScalar::InExternC(Context<FieldDescriptor> field) const {
  field.Emit(
      {{"Scalar", PrimitiveRsTypeName(field.desc())},
       {"hazzer_thunk", Thunk(field, "has")},
       {"mutable_upb_array_thunk", Thunk(field, "mutable_upb_array")},
       {"mutable_upb_array",
        [&] {
          if (field.is_upb()) {
            field.Emit(R"rs(
            fn _$mutable_upb_array_thunk$(raw_msg: $pbi$::RawMessage, size: *const usize, arena: $pbi$::RawArena) -> $pbi$::RawRepeatedField;
          )rs");
          } else {
            field.Emit(R"rs(
            fn _$mutable_upb_array_thunk$(raw_msg: $pbi$::RawMessage) -> $pbi$::RawRepeatedField;
          )rs");
          }
        }},
       {"clearer_thunk", Thunk(field, "clear")},
       {"hazzer",
        [&] {
          if (field.desc().has_presence()) {
            field.Emit(
                R"rs(fn $hazzer_thunk$(raw_msg: $pbi$::RawMessage) -> bool;)rs");
          }
        }}},
      R"rs(
          $hazzer$
          fn $clearer_thunk$(raw_msg: $pbi$::RawMessage);
          $mutable_upb_array$
        )rs");
}

void RepeatedScalar::InThunkCc(Context<FieldDescriptor> field) const {
  field.Emit(
      {{"field", cpp::FieldName(&field.desc())},
       {"Scalar", cpp::PrimitiveTypeName(field.desc().cpp_type())},
       {"QualifiedMsg",
        cpp::QualifiedClassName(field.desc().containing_type())},
       {"hazzer_thunk", Thunk(field, "has")},
       {"clearer_thunk", Thunk(field, "clear")},
       {"hazzer",
        [&] {
          if (field.desc().has_presence()) {
            field.Emit(R"cc(
              bool $hazzer_thunk$($QualifiedMsg$* msg) {
                return msg->has_$field$();
              }
            )cc");
          }
        }},
       {"mutable_upb_array_thunk", Thunk(field, "mutable_upb_array")},
       {"impls",
        [&] {
          field.Emit(
              R"cc(
                $hazzer$;
                void $clearer_thunk$($QualifiedMsg$* msg) { msg->clear_$field$(); }
                google::protobuf::RepeatedField<$Scalar$>* _$mutable_upb_array_thunk$(
                    $QualifiedMsg$* msg) {
                  return msg->mutable_$field$();
                }
              )cc");
        }}},
      "$impls$");
}

}  // namespace rust
}  // namespace compiler
}  // namespace protobuf
}  // namespace google
