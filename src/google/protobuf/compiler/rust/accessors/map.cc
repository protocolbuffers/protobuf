// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include <string>

#include "google/protobuf/compiler/cpp/helpers.h"
#include "google/protobuf/compiler/rust/accessors/accessor_case.h"
#include "google/protobuf/compiler/rust/accessors/generator.h"
#include "google/protobuf/compiler/rust/context.h"
#include "google/protobuf/compiler/rust/naming.h"
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

  ctx.Emit({{"field", RsSafeName(field.name())},
            {"Key", RsTypePath(ctx, key_type)},
            {"Value", RsTypePath(ctx, value_type)},
            {"view_lifetime", ViewLifetime(accessor_case)},
            {"view_self", ViewReceiver(accessor_case)},
            {"getter_thunk", ThunkName(ctx, field, "get")},
            {"getter_mut_thunk", ThunkName(ctx, field, "get_mut")},
            {"getter",
             [&] {
               if (ctx.is_upb()) {
                 ctx.Emit({}, R"rs(
                    pub fn $field$($view_self$)
                      -> $pb$::MapView<$view_lifetime$, $Key$, $Value$> {
                      unsafe {
                        $getter_thunk$(self.raw_msg())
                          .map_or_else(
                            $pbr$::empty_map::<$Key$, $Value$>,
                            |raw| $pb$::MapView::from_raw($pbi$::Private, raw)
                          )
                      }
                    })rs");
               } else {
                 ctx.Emit({}, R"rs(
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
                      let raw = unsafe {
                        $getter_mut_thunk$(self.raw_msg(),
                                           self.arena().raw())
                      };
                      let inner = $pbr$::InnerMapMut::new($pbi$::Private,
                        raw, self.arena());
                      unsafe { $pb$::MapMut::from_inner($pbi$::Private, inner) }
                    })rs");
               } else {
                 ctx.Emit({}, R"rs(
                    pub fn $field$_mut(&mut self)
                      -> $pb$::MapMut<'_, $Key$, $Value$> {
                      let inner = $pbr$::InnerMapMut::new($pbi$::Private,
                        unsafe { $getter_mut_thunk$(self.raw_msg()) });
                      unsafe { $pb$::MapMut::from_inner($pbi$::Private, inner) }
                    })rs");
               }
             }}},
           R"rs(
    $getter$
    $getter_mut$
    )rs");
}

void Map::InExternC(Context& ctx, const FieldDescriptor& field) const {
  ctx.Emit(
      {
          {"getter_thunk", ThunkName(ctx, field, "get")},
          {"getter_mut_thunk", ThunkName(ctx, field, "get_mut")},
          {"getter",
           [&] {
             if (ctx.is_upb()) {
               ctx.Emit({}, R"rs(
                fn $getter_thunk$(raw_msg: $pbr$::RawMessage)
                  -> Option<$pbr$::RawMap>;
                fn $getter_mut_thunk$(raw_msg: $pbr$::RawMessage,
                  arena: $pbr$::RawArena) -> $pbr$::RawMap;
              )rs");
             } else {
               ctx.Emit({}, R"rs(
                fn $getter_thunk$(msg: $pbr$::RawMessage) -> $pbr$::RawMap;
                fn $getter_mut_thunk$(msg: $pbr$::RawMessage,) -> $pbr$::RawMap;
              )rs");
             }
           }},
      },
      R"rs(
    $getter$
  )rs");
}

void Map::InThunkCc(Context& ctx, const FieldDescriptor& field) const {
  ctx.Emit({{"field", cpp::FieldName(&field)},
            {"Key", MapElementTypeName(*field.message_type()->map_key())},
            {"Value", MapElementTypeName(*field.message_type()->map_value())},
            {"QualifiedMsg", cpp::QualifiedClassName(field.containing_type())},
            {"getter_thunk", ThunkName(ctx, field, "get")},
            {"getter_mut_thunk", ThunkName(ctx, field, "get_mut")},
            {"impls",
             [&] {
               ctx.Emit(
                   R"cc(
                     const void* $getter_thunk$(const $QualifiedMsg$* msg) {
                       return &msg->$field$();
                     }
                     void* $getter_mut_thunk$($QualifiedMsg$* msg) { return msg->mutable_$field$(); }
                   )cc");
             }}},
           "$impls$");
}

}  // namespace rust
}  // namespace compiler
}  // namespace protobuf
}  // namespace google
