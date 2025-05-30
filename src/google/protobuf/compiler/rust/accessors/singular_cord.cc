// Protocol Buffers - Google's data interchange format
// Copyright 2024 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include <string>

#include "absl/log/absl_check.h"
#include "absl/strings/string_view.h"
#include "google/protobuf/compiler/cpp/helpers.h"
#include "google/protobuf/compiler/rust/accessors/accessor_case.h"
#include "google/protobuf/compiler/rust/accessors/default_value.h"
#include "google/protobuf/compiler/rust/accessors/generator.h"
#include "google/protobuf/compiler/rust/accessors/with_presence.h"
#include "google/protobuf/compiler/rust/context.h"
#include "google/protobuf/compiler/rust/naming.h"
#include "google/protobuf/compiler/rust/upb_helpers.h"
#include "google/protobuf/descriptor.h"

namespace google {
namespace protobuf {
namespace compiler {
namespace rust {

void SingularCord::InMsgImpl(Context& ctx, const FieldDescriptor& field,
                             AccessorCase accessor_case) const {
  if (field.has_presence()) {
    WithPresenceAccessorsInMsgImpl(ctx, field, accessor_case);
  }

  std::string field_name = FieldNameWithCollisionAvoidance(field);
  bool is_string_type = field.type() == FieldDescriptor::TYPE_STRING;
  ctx.Emit({{"field", RsSafeName(field_name)},
            {"raw_field_name", field_name},
            {"proxied_type", RsTypePath(ctx, field)},
            {"default_value", DefaultValue(ctx, field)},
            {"upb_mt_field_index", UpbMiniTableFieldIndex(field)},
            {"borrowed_type",
             [&] {
               if (is_string_type) {
                 ctx.Emit("$pb$::ProtoStr");
               } else {
                 ctx.Emit("[u8]");
               }
             }},
            {"transform_borrowed",
             [&] {
               if (is_string_type) {
                 ctx.Emit(R"rs(
                $pb$::ProtoStringCow::Borrowed(
                  // SAFETY: The runtime doesn't require ProtoStr to be UTF-8.
                  unsafe { $pb$::ProtoStr::from_utf8_unchecked(view.as_ref()) }
                )
              )rs");
               } else {
                 ctx.Emit(R"rs(
                $pb$::ProtoBytesCow::Borrowed(
                  unsafe { view.as_ref() }
                )
               )rs");
               }
             }},
            {"transform_owned",
             [&] {
               if (is_string_type) {
                 ctx.Emit(R"rs(
                $pb$::ProtoStringCow::Owned(
                  $pb$::ProtoString::from_inner($pbi$::Private, inner)
                )
              )rs");
               } else {
                 ctx.Emit(R"rs(
                $pb$::ProtoBytesCow::Owned(
                  $pb$::ProtoBytes::from_inner($pbi$::Private, inner)
                )
              )rs");
               }
             }},
            {"view_lifetime", ViewLifetime(accessor_case)},
            {"view_type",
             [&] {
               if (is_string_type) {
                 ctx.Emit("$pb$::ProtoStringCow<$view_lifetime$>");
               } else {
                 ctx.Emit("$pb$::ProtoBytesCow<$view_lifetime$>");
               }
             }},
            {"view_self", ViewReceiver(accessor_case)},
            {"getter_impl",
             [&] {
               if (ctx.is_cpp()) {
                 ctx.Emit(
                     {{"is_flat_thunk", ThunkName(ctx, field, "cord_is_flat")},
                      {"borrowed_getter_thunk",
                       ThunkName(ctx, field, "get_cord_borrowed")},
                      {"owned_getter_thunk",
                       ThunkName(ctx, field, "get_cord_owned")}},
                     R"rs(
                  let cord_is_flat = unsafe { $is_flat_thunk$(self.raw_msg()) };
                  if cord_is_flat {
                    let view = unsafe { $borrowed_getter_thunk$(self.raw_msg()) };
                    return $transform_borrowed$;
                  }

                  let owned = unsafe { $owned_getter_thunk$(self.raw_msg()) };
                  let inner = unsafe { $pbr$::InnerProtoString::from_raw(owned) };

                  $transform_owned$
                )rs");
               } else {
                 ctx.Emit(R"rs(
                let view = unsafe {
                  let f = $pbr$::upb_MiniTable_GetFieldByIndex(
                      <Self as $pbr$::AssociatedMiniTable>::mini_table(),
                      $upb_mt_field_index$);
                  $pbr$::upb_Message_GetString(
                      self.raw_msg(), f, ($default_value$).into())
                };
                $transform_borrowed$
              )rs");
               }
             }},
            {"getter",
             [&] {
               ctx.Emit(R"rs(
                pub fn $field$($view_self$) -> $view_type$ {
                  $getter_impl$
                }
            )rs");
             }},
            {"setter_impl",
             [&] {
               if (ctx.is_cpp()) {
                 ctx.Emit({{"setter_thunk", ThunkName(ctx, field, "set")}},
                          R"rs(
              let s = val.into_proxied($pbi$::Private);
              unsafe {
                $setter_thunk$(
                  self.inner.msg(),
                  s.into_inner($pbi$::Private).into_raw()
                );
              }
            )rs");
               } else {
                 ctx.Emit(R"rs(
              let s = val.into_proxied($pbi$::Private);
              let (view, arena) =
                s.into_inner($pbi$::Private).into_raw_parts();

              let parent_arena = self.inner.arena();
              parent_arena.fuse(&arena);

              unsafe {
                let f = $pbr$::upb_MiniTable_GetFieldByIndex(
                          <Self as $pbr$::AssociatedMiniTable>::mini_table(),
                          $upb_mt_field_index$);
                $pbr$::upb_Message_SetBaseFieldString(
                  self.inner.msg(),
                  f,
                  view);
              }
            )rs");
               }
             }},
            {"setter",
             [&] {
               if (accessor_case == AccessorCase::VIEW) return;
               ctx.Emit({},
                        R"rs(
              pub fn set_$raw_field_name$(&mut self, val: impl $pb$::IntoProxied<$proxied_type$>) {
                $setter_impl$
              }
            )rs");
             }}},
           R"rs(
        $getter$
        $setter$
      )rs");
}

void SingularCord::InExternC(Context& ctx, const FieldDescriptor& field) const {
  ABSL_CHECK(ctx.is_cpp());

  if (field.has_presence()) {
    WithPresenceAccessorsInExternC(ctx, field);
  }

  ctx.Emit(
      {{"borrowed_getter_thunk", ThunkName(ctx, field, "get_cord_borrowed")},
       {"owned_getter_thunk", ThunkName(ctx, field, "get_cord_owned")},
       {"is_flat_thunk", ThunkName(ctx, field, "cord_is_flat")},
       {"getter_thunk", ThunkName(ctx, field, "get")},
       {"setter_thunk", ThunkName(ctx, field, "set")},
       {"setter",
        [&] {
          if (ctx.is_cpp()) {
            ctx.Emit(R"rs(
              fn $setter_thunk$(raw_msg: $pbr$::RawMessage, val: $pbr$::CppStdString);
            )rs");
          } else {
            ctx.Emit(R"rs(
              fn $setter_thunk$(raw_msg: $pbr$::RawMessage, val: $pbr$::PtrAndLen);
            )rs");
          }
        }},
       {"getter_thunks",
        [&] {
          if (ctx.is_cpp()) {
            ctx.Emit(R"rs(
             fn $is_flat_thunk$(raw_msg: $pbr$::RawMessage) -> bool;
             fn $borrowed_getter_thunk$(raw_msg: $pbr$::RawMessage) -> $pbr$::PtrAndLen;
             fn $owned_getter_thunk$(raw_msg: $pbr$::RawMessage) -> $pbr$::CppStdString;
           )rs");
          } else {
            ctx.Emit({{"getter_thunk", ThunkName(ctx, field, "get")}}, R"rs(
             fn $getter_thunk$(raw_msg: $pbr$::RawMessage) -> $pbr$::PtrAndLen;
           )rs");
          }
        }}},
      R"rs(
          $getter_thunks$
          $setter$
        )rs");
}

void SingularCord::InThunkCc(Context& ctx, const FieldDescriptor& field) const {
  ABSL_CHECK(ctx.is_cpp());

  if (field.has_presence()) {
    WithPresenceAccessorsInThunkCc(ctx, field);
  }

  ctx.Emit(
      {{"field", cpp::FieldName(&field)},
       {"QualifiedMsg", cpp::QualifiedClassName(field.containing_type())},
       {"setter_thunk", ThunkName(ctx, field, "set")},
       {"borrowed_getter_thunk", ThunkName(ctx, field, "get_cord_borrowed")},
       {"owned_getter_thunk", ThunkName(ctx, field, "get_cord_owned")},
       {"is_flat_thunk", ThunkName(ctx, field, "cord_is_flat")}},
      R"cc(
        bool $is_flat_thunk$($QualifiedMsg$* msg) {
          const absl::Cord& cord = msg->$field$();
          return cord.TryFlat().has_value();
        }
        ::google::protobuf::rust::PtrAndLen $borrowed_getter_thunk$($QualifiedMsg$* msg) {
          const absl::Cord& cord = msg->$field$();
          absl::string_view s = cord.TryFlat().value();
          return ::google::protobuf::rust::PtrAndLen{s.data(), s.size()};
        }
        std::string* $owned_getter_thunk$($QualifiedMsg$* msg) {
          const absl::Cord& cord = msg->$field$();
          std::string* owned = new std::string();
          absl::CopyCordToString(cord, owned);
          return owned;
        }
        void $setter_thunk$($QualifiedMsg$* msg, std::string* s) {
          msg->set_$field$(absl::Cord(std::move(*s)));
          delete s;
        }
      )cc");
}

}  // namespace rust
}  // namespace compiler
}  // namespace protobuf
}  // namespace google
