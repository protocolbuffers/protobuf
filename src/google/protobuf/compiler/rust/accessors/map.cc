// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include <string>

#include "absl/log/absl_check.h"
#include "google/protobuf/compiler/cpp/helpers.h"
#include "google/protobuf/compiler/rust/accessors/accessor_case.h"
#include "google/protobuf/compiler/rust/accessors/generator.h"
#include "google/protobuf/compiler/rust/context.h"
#include "google/protobuf/compiler/rust/naming.h"
#include "google/protobuf/compiler/rust/upb_helpers.h"
#include "google/protobuf/descriptor.h"
#include "google/protobuf/descriptor.pb.h"

namespace google {
namespace protobuf {
namespace compiler {
namespace rust {
namespace {

std::string MapElementTypeName(const FieldDescriptor& field) {
  auto cpp_type = field.cpp_type();
  switch (cpp_type) {
    case FieldDescriptor::CPPTYPE_MESSAGE:
      return cpp::QualifiedClassName(field.message_type());
    case FieldDescriptor::CPPTYPE_ENUM:
      return cpp::QualifiedClassName(field.enum_type());
    default:
      return cpp::PrimitiveTypeName(cpp_type);
  }
}

}  // namespace

void Map::InMsgImpl(Context& ctx, const FieldDescriptor& field,
                    AccessorCase accessor_case) const {
  auto& key_type = *field.message_type()->map_key();
  auto& value_type = *field.message_type()->map_value();
  std::string field_name = FieldNameWithCollisionAvoidance(field);

  ctx.Emit(
      {{"field", RsSafeName(field_name)},
       {"raw_field_name", field_name},  // Never r# prefixed
       {"Key", RsTypePath(ctx, key_type)},
       {"Value", RsTypePath(ctx, value_type)},
       {"view_lifetime", ViewLifetime(accessor_case)},
       {"view_self", ViewReceiver(accessor_case)},
       {"upb_mt_field_index", UpbMiniTableFieldIndex(field)},
       {"getter",
        [&] {
          if (ctx.is_upb()) {
            ctx.Emit(R"rs(
                    pub fn $field$($view_self$)
                      -> $pb$::MapView<$view_lifetime$, $Key$, $Value$> {
                      unsafe {
                        let f = $pbr$::upb_MiniTable_GetFieldByIndex(
                          <Self as $pbr$::AssociatedMiniTable>::mini_table(),
                          $upb_mt_field_index$);
                        $pbr$::upb_Message_GetMap(self.raw_msg(), f)
                          .map_or_else(
                            $pbr$::empty_map::<$Key$, $Value$>,
                            |raw| $pb$::MapView::from_raw($pbi$::Private, raw)
                          )
                      }
                    })rs");
          } else {
            ctx.Emit({{"getter_thunk", ThunkName(ctx, field, "get")}}, R"rs(
                    pub fn $field$($view_self$)
                      -> $pb$::MapView<$view_lifetime$, $Key$, $Value$> {
                      unsafe {
                        $pb$::MapView::from_raw($pbi$::Private,
                          $getter_thunk$(self.raw_msg()))
                      }
                    })rs");
          }
        }},
       {"getter_mut",
        [&] {
          if (accessor_case == AccessorCase::VIEW) {
            return;
          }
          if (ctx.is_upb()) {
            ctx.Emit({}, R"rs(
                    pub fn $field$_mut(&mut self)
                      -> $pb$::MapMut<'_, $Key$, $Value$> {
                      unsafe {
                        let parent_mini_table =
                          <Self as $pbr$::AssociatedMiniTable>::mini_table();

                        let f =
                          $pbr$::upb_MiniTable_GetFieldByIndex(
                              parent_mini_table,
                              $upb_mt_field_index$);

                        let map_entry_mini_table =
                          $pbr$::upb_MiniTable_SubMessage(
                              parent_mini_table,
                              f);

                        let raw_map =
                          $pbr$::upb_Message_GetOrCreateMutableMap(
                              self.raw_msg(),
                              map_entry_mini_table,
                              f,
                              self.arena().raw()).unwrap();
                        let inner = $pbr$::InnerMapMut::new(
                          raw_map, self.arena());
                        $pb$::MapMut::from_inner($pbi$::Private, inner)
                      }
                    })rs");
          } else {
            ctx.Emit({{"getter_mut_thunk", ThunkName(ctx, field, "get_mut")}},
                     R"rs(
                    pub fn $field$_mut(&mut self)
                      -> $pb$::MapMut<'_, $Key$, $Value$> {
                      let inner = $pbr$::InnerMapMut::new(
                        unsafe { $getter_mut_thunk$(self.raw_msg()) });
                      unsafe { $pb$::MapMut::from_inner($pbi$::Private, inner) }
                    })rs");
          }
        }},
       {"setter",
        [&] {
          if (accessor_case == AccessorCase::VIEW) {
            return;
          }
          if (ctx.is_upb()) {
            ctx.Emit({}, R"rs(
                  pub fn set_$raw_field_name$(
                      &mut self,
                      src: impl $pb$::IntoProxied<$pb$::Map<$Key$, $Value$>>) {
                    let minitable_field = unsafe {
                      $pbr$::upb_MiniTable_GetFieldByIndex(
                        <Self as $pbr$::AssociatedMiniTable>::mini_table(),
                        $upb_mt_field_index$
                      )
                    };
                    let mut val = src.into_proxied($pbi$::Private);
                    let val_as_mut = val.as_mut();
                    let mut inner = val_as_mut.inner($pbi$::Private);

                    self.arena().fuse(inner.arena());
                    unsafe {
                        let value_ptr: *const *const $std$::ffi::c_void =
                            &(inner.as_raw().as_ptr() as *const $std$::ffi::c_void);
                        $pbr$::upb_Message_SetBaseField(self.raw_msg(),
                          minitable_field,
                          value_ptr as *const $std$::ffi::c_void);
                    }
                  }
                )rs");
          } else {
            ctx.Emit({{"move_setter_thunk", ThunkName(ctx, field, "set")}},
                     R"rs(
                  pub fn set_$raw_field_name$(
                      &mut self,
                      src: impl $pb$::IntoProxied<$pb$::Map<$Key$, $Value$>>) {
                    let val = $std$::mem::ManuallyDrop::new(
                        src.into_proxied($pbi$::Private));
                    unsafe {
                      $move_setter_thunk$(
                          self.raw_msg(),
                          val.as_raw($pbi$::Private));
                    }
                  }
                )rs");
          }
        }}},
      R"rs(
    $getter$
    $getter_mut$
    $setter$
    )rs");
}

void Map::InExternC(Context& ctx, const FieldDescriptor& field) const {
  ABSL_CHECK(ctx.is_cpp());

  ctx.Emit(
      {
          {"getter_thunk", ThunkName(ctx, field, "get")},
          {"getter_mut_thunk", ThunkName(ctx, field, "get_mut")},
          {"move_setter_thunk", ThunkName(ctx, field, "set")},
          {"getter",
           [&] {
             if (ctx.is_upb()) {
               ctx.Emit({}, R"rs(
                fn $getter_thunk$(raw_msg: $pbr$::RawMessage)
                  -> $Option$<$pbr$::RawMap>;
                fn $getter_mut_thunk$(raw_msg: $pbr$::RawMessage,
                  arena: $pbr$::RawArena) -> $pbr$::RawMap;
              )rs");
             } else {
               ctx.Emit({}, R"rs(
                fn $getter_thunk$(msg: $pbr$::RawMessage) -> $pbr$::RawMap;
                fn $getter_mut_thunk$(msg: $pbr$::RawMessage,) -> $pbr$::RawMap;
                fn $move_setter_thunk$(
                    raw_msg: $pbr$::RawMessage,
                    value: $pbr$::RawMap);
              )rs");
             }
           }},
      },
      R"rs(
    $getter$
  )rs");
}

void Map::InThunkCc(Context& ctx, const FieldDescriptor& field) const {
  ABSL_CHECK(ctx.is_cpp());

  ctx.Emit(
      {{"field", cpp::FieldName(&field)},
       {"Key", MapElementTypeName(*field.message_type()->map_key())},
       {"Value", MapElementTypeName(*field.message_type()->map_value())},
       {"QualifiedMsg", cpp::QualifiedClassName(field.containing_type())},
       {"getter_thunk", ThunkName(ctx, field, "get")},
       {"getter_mut_thunk", ThunkName(ctx, field, "get_mut")},
       {"move_setter_thunk", ThunkName(ctx, field, "set")},
       {"impls",
        [&] {
          ctx.Emit(
              R"cc(
                const void* $getter_thunk$(const $QualifiedMsg$* msg) {
                  return &msg->$field$();
                }
                void* $getter_mut_thunk$($QualifiedMsg$* msg) { return msg->mutable_$field$(); }
                void $move_setter_thunk$($QualifiedMsg$* msg,
                                         google::protobuf::Map<$Key$, $Value$>* value) {
                  *msg->mutable_$field$() = std::move(*value);
                  delete value;
                }
              )cc");
        }}},
      "$impls$");
}

}  // namespace rust
}  // namespace compiler
}  // namespace protobuf
}  // namespace google
