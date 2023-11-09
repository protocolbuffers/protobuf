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

void RepeatedMessage::InMsgImpl(Context<FieldDescriptor> field) const {
  auto prefix = field.desc().containing_type()->name();

  field.Emit(
      {
          {"field", field.desc().name()},
          {"prefix", prefix},
          {"FieldMsg", field.desc().name()},
          {"getter_thunk", Thunk(field, "get")},
          {"getter_mut_thunk", Thunk(field, "get_mut")},
          {"getters",
           [&] {
             if (field.is_upb()) {
               field.Emit(R"rs(
pub fn r#$field$(&self) -> $pb$::RepeatedView<'_, $prefix$> {
  let inner = unsafe { $getter_thunk$(
      self.inner.msg,
      /* optional size pointer */ std::ptr::null(),
    )}
    .map_or_else(|| unsafe {$pbr$::empty_array()}, |raw| {
      $pbr$::RepeatedFieldInner{ raw, arena: &self.inner.arena }
    });
  $pb$::RepeatedView::from_inner( $pbi$::Private, inner)
}
pub fn r#$field$_mut(&mut self) -> $pb$::RepeatedMut<'_, $prefix$> {
    $pb$::RepeatedMut::from_inner($pbi$::Private, $pbr$::RepeatedFieldInner{
      raw: unsafe { $getter_mut_thunk$(
        self.inner.msg, std::ptr::null(), self.inner.arena.raw())},
      arena: &self.inner.arena}
    )
}
  )rs");
             } else {
               field.Emit(R"rs(
pub fn r#$field$(&self) -> $pb$::RepeatedView<'_, $prefix$> {
    $pb$::RepeatedView::from_inner($pbi$::Private, $pbr$::RepeatedFieldInner{
      raw: unsafe { $getter_thunk$(self.inner.msg)},
      _phantom: std::marker::PhantomData} )
}
pub fn r#$field$_mut(&mut self) -> $pb$::RepeatedMut<'_, $prefix$> {
    $pb$::RepeatedMut::from_inner($pbi$::Private, $pbr$::RepeatedFieldInner{
      raw: unsafe { $getter_mut_thunk$(self.inner.msg)},
      _phantom: std::marker::PhantomData} )
}
  )rs");
             }
           }},
      },
      "$getters$");
}
void RepeatedMessage::InExternC(Context<FieldDescriptor> field) const {
  field.Emit(
      {
          {"getter_thunk", Thunk(field, "get")},
          {"getter_mut_thunk", Thunk(field, "get_mut")},
          {
              "getter",
              [&] {
                if (field.is_upb()) {
                  field.Emit(R"rs(
fn $getter_mut_thunk$(
  raw_msg: $pbi$::RawMessage, size: *const usize, arena: $pbi$::RawArena,
) -> $pbi$::RawRepeatedField;

//  Returns `None` when returned array pointer is NULL.
fn $getter_thunk$(
  raw_msg: $pbi$::RawMessage, size: *const usize,
) -> Option<$pbi$::RawRepeatedField>;
          )rs");
                } else {
                  field.Emit(R"rs(
fn $getter_mut_thunk$(raw_msg: $pbi$::RawMessage) -> $pbi$::RawRepeatedField;
fn $getter_thunk$(raw_msg: $pbi$::RawMessage) -> $pbi$::RawRepeatedField;
          )rs");
                }
              },
          },
      },
      R"rs(
          $getter$
        )rs");
}

void RepeatedMessage::InThunkCc(Context<FieldDescriptor> field) const {
  field.Emit(
      {
          {"repeated_get_thunk", Thunk(field, "repeated_get")},
          {"repeated_len_thunk", Thunk(field, "repeated_len")},
          {"QualifiedField",
           cpp::QualifiedClassName(field.desc().message_type())},
          {"QualifiedMsg",
           cpp::QualifiedClassName(field.desc().containing_type())},
          {"field", cpp::FieldName(&field.desc())},
          {"getter_thunk", Thunk(field, "get")},
          {"getter_mut_thunk", Thunk(field, "get_mut")},
      },
      R"cc(
        google::protobuf::RepeatedPtrField<$QualifiedField$> const& $getter_thunk$(
            $QualifiedMsg$* msg) {
          return msg->$field$();
        }
        google::protobuf::RepeatedPtrField<$QualifiedField$>* $getter_mut_thunk$(
            $QualifiedMsg$* msg) {
          return msg->mutable_$field$();
        }
      )cc");
}

}  // namespace rust
}  // namespace compiler
}  // namespace protobuf
}  // namespace google
