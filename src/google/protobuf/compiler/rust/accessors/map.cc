// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "google/protobuf/compiler/cpp/helpers.h"
#include "google/protobuf/compiler/rust/accessors/accessor_generator.h"
#include "google/protobuf/compiler/rust/context.h"
#include "google/protobuf/compiler/rust/naming.h"
#include "google/protobuf/descriptor.h"
#include "google/protobuf/descriptor.pb.h"

namespace google {
namespace protobuf {
namespace compiler {
namespace rust {

void Map::InMsgImpl(Context<FieldDescriptor> field) const {
  auto& key_type = *field.desc().message_type()->map_key();
  auto& value_type = *field.desc().message_type()->map_value();

  field.Emit({{"field", field.desc().name()},
              {"Key", PrimitiveRsTypeName(key_type)},
              {"Value", PrimitiveRsTypeName(value_type)},
              {"getter_thunk", Thunk(field, "get")},
              {"getter_mut_thunk", Thunk(field, "get_mut")},
              {"getter",
               [&] {
                 if (field.is_upb()) {
                   field.Emit({}, R"rs(
                    pub fn r#$field$(&self)
                      -> $pb$::View<'_, $pb$::Map<$Key$, $Value$>> {
                      let inner = unsafe {
                        $getter_thunk$(self.inner.msg)
                      }.map_or_else(|| unsafe {$pbr$::empty_map()}, |raw| {
                        $pbr$::MapInner{
                          raw,
                          arena: &self.inner.arena,
                          _phantom_key: std::marker::PhantomData,
                          _phantom_value: std::marker::PhantomData,
                        }
                      });
                      $pb$::MapView::from_inner($pbi$::Private, inner)
                    })rs");
                 } else {
                   field.Emit({}, R"rs(
                    pub fn r#$field$(&self)
                      -> $pb$::View<'_, $pb$::Map<$Key$, $Value$>> {
                      let inner = $pbr$::MapInner {
                        raw: unsafe { $getter_thunk$(self.inner.msg) },
                        _phantom_key: std::marker::PhantomData,
                        _phantom_value: std::marker::PhantomData,
                      };
                      $pb$::MapView::from_inner($pbi$::Private, inner)
                    })rs");
                 }
               }},
              {"getter_mut",
               [&] {
                 if (field.is_upb()) {
                   field.Emit({}, R"rs(
                    pub fn r#$field$_mut(&mut self)
                      -> $pb$::Mut<'_, $pb$::Map<$Key$, $Value$>> {
                      let raw = unsafe {
                        $getter_mut_thunk$(self.inner.msg,
                                           self.inner.arena.raw())
                      };
                      let inner = $pbr$::MapInner{
                        raw,
                        arena: &self.inner.arena,
                        _phantom_key: std::marker::PhantomData,
                        _phantom_value: std::marker::PhantomData,
                      };
                      $pb$::MapMut::from_inner($pbi$::Private, inner)
                    })rs");
                 } else {
                   field.Emit({}, R"rs(
                    pub fn r#$field$_mut(&mut self)
                      -> $pb$::Mut<'_, $pb$::Map<$Key$, $Value$>> {
                      let inner = $pbr$::MapInner {
                        raw: unsafe { $getter_mut_thunk$(self.inner.msg) },
                        _phantom_key: std::marker::PhantomData,
                        _phantom_value: std::marker::PhantomData,
                      };
                      $pb$::MapMut::from_inner($pbi$::Private, inner)
                    })rs");
                 }
               }}},
             R"rs(
    $getter$
    $getter_mut$
    )rs");
}

void Map::InExternC(Context<FieldDescriptor> field) const {
  field.Emit(
      {
          {"getter_thunk", Thunk(field, "get")},
          {"getter_mut_thunk", Thunk(field, "get_mut")},
          {"getter",
           [&] {
             if (field.is_upb()) {
               field.Emit({}, R"rs(
                fn $getter_thunk$(raw_msg: $pbi$::RawMessage)
                  -> Option<$pbi$::RawMap>;
                fn $getter_mut_thunk$(raw_msg: $pbi$::RawMessage,
                  arena: $pbi$::RawArena) -> $pbi$::RawMap;
              )rs");
             } else {
               field.Emit({}, R"rs(
                fn $getter_thunk$(msg: $pbi$::RawMessage) -> $pbi$::RawMap;
                fn $getter_mut_thunk$(msg: $pbi$::RawMessage,) -> $pbi$::RawMap;
              )rs");
             }
           }},
      },
      R"rs(
    $getter$
  )rs");
}

void Map::InThunkCc(Context<FieldDescriptor> field) const {
  field.Emit(
      {{"field", cpp::FieldName(&field.desc())},
       {"Key", cpp::PrimitiveTypeName(
                   field.desc().message_type()->map_key()->cpp_type())},
       {"Value", cpp::PrimitiveTypeName(
                     field.desc().message_type()->map_value()->cpp_type())},
       {"QualifiedMsg",
        cpp::QualifiedClassName(field.desc().containing_type())},
       {"getter_thunk", Thunk(field, "get")},
       {"getter_mut_thunk", Thunk(field, "get_mut")},
       {"impls",
        [&] {
          field.Emit(
              R"cc(
                const void* $getter_thunk$($QualifiedMsg$& msg) {
                  return &msg.$field$();
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
