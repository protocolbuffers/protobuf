// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

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
              {"getter_thunk_mut", Thunk(field, "get_mut")},
              {"getter",
               [&] {
                 if (field.is_upb()) {
                   field.Emit({}, R"rs(
                pub fn r#$field$(&self) -> $pb$::MapView<'_, $Key$, $Value$> {
                  let inner = unsafe {
                    $getter_thunk$(self.inner.msg)
                  }.map_or_else(|| unsafe {$pbr$::empty_map()}, |raw| {
                    $pbr$::MapInner{ raw, arena: &self.inner.arena }
                  });
                  $pb$::MapView::from_inner($pbi$::Private, inner)
                }

                pub fn r#$field$_mut(&mut self) 
                  -> $pb$::MapMut<'_, $Key$, $Value$> {
                  let raw = unsafe {
                    $getter_thunk_mut$(self.inner.msg, self.inner.arena.raw())
                  };
                  let inner = $pbr$::MapInner{ raw, arena: &self.inner.arena };
                  $pb$::MapMut::from_inner($pbi$::Private, inner)
                }
              )rs");
                 }
               }}},
             R"rs(
    $getter$
    )rs");
}

void Map::InExternC(Context<FieldDescriptor> field) const {
  field.Emit(
      {
          {"getter_thunk", Thunk(field, "get")},
          {"getter_thunk_mut", Thunk(field, "get_mut")},
          {"getter",
           [&] {
             if (field.is_upb()) {
               field.Emit({}, R"rs(
          fn $getter_thunk$(raw_msg: $pbi$::RawMessage) 
            -> Option<$pbi$::RawMap>;
          fn $getter_thunk_mut$(raw_msg: $pbi$::RawMessage,
            arena: $pbi$::RawArena) -> $pbi$::RawMap;
        )rs");
             }
           }},
      },
      R"rs(
    $getter$
  )rs");
}

}  // namespace rust
}  // namespace compiler
}  // namespace protobuf
}  // namespace google
