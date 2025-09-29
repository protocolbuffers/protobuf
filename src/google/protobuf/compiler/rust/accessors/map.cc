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
                        <Self as $pbr$::UpbGetMessagePtr>::get_ptr(&self, $pbi$::Private)
                          .get_map_at_index($upb_mt_field_index$)
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
                        let raw_map = <Self as $pbr$::UpbGetMessagePtrMut>::get_ptr_mut(self, $pbi$::Private)
                          .get_or_create_mutable_map_at_index(
                            $upb_mt_field_index$, self.inner.arena()).unwrap();
                        let inner = $pbr$::InnerMapMut::new(
                          raw_map, self.inner.arena());
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
                    let mut val = src.into_proxied($pbi$::Private);
                    let val_as_mut = val.as_mut();
                    let mut inner = val_as_mut.inner($pbi$::Private);

                    self.inner.arena().fuse(inner.arena());
                    unsafe {
                        <Self as $pbr$::UpbGetMessagePtrMut>::get_ptr_mut(self, $pbi$::Private)
                            .set_map_at_index(
                                $upb_mt_field_index$,
                                inner.as_raw());
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
