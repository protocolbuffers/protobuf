// Protocol Buffers - Google's data interchange format
// Copyright 2024 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include <string>

#include "absl/strings/string_view.h"
#include "google/protobuf/compiler/cpp/helpers.h"
#include "google/protobuf/compiler/rust/accessors/accessor_case.h"
#include "google/protobuf/compiler/rust/accessors/generator.h"
#include "google/protobuf/compiler/rust/context.h"
#include "google/protobuf/compiler/rust/naming.h"
#include "google/protobuf/descriptor.h"

namespace google {
namespace protobuf {
namespace compiler {
namespace rust {

void SingularCord::InMsgImpl(Context& ctx, const FieldDescriptor& field,
                             AccessorCase accessor_case) const {
  std::string field_name = FieldNameWithCollisionAvoidance(field);
  bool is_string_type = field.type() == FieldDescriptor::TYPE_STRING;
  ctx.Emit(
      {{"field", RsSafeName(field_name)},
       {"raw_field_name", field_name},
       {"hazzer_thunk", ThunkName(ctx, field, "has")},
       {"borrowed_getter_thunk", ThunkName(ctx, field, "get_cord_borrowed")},
       {"owned_getter_thunk", ThunkName(ctx, field, "get_cord_owned")},
       {"is_flat_thunk", ThunkName(ctx, field, "cord_is_flat")},
       {"setter_thunk", ThunkName(ctx, field, "set")},
       {"clearer_thunk", ThunkName(ctx, field, "clear")},
       {"getter_thunk", ThunkName(ctx, field, "get")},
       {"proxied_type", RsTypePath(ctx, field)},
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
                  unsafe { $pb$::ProtoStr::from_utf8_unchecked(view) }
                )
              )rs");
          } else {
            ctx.Emit(R"rs(
                $pb$::ProtoBytesCow::Borrowed(
                  view
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
            ctx.Emit(R"rs(
                  let cord_is_flat = unsafe { $is_flat_thunk$(self.raw_msg()) };
                  if cord_is_flat {
                    let view = unsafe { $borrowed_getter_thunk$(self.raw_msg()).as_ref() };
                    return $transform_borrowed$;
                  }

                  let owned = unsafe { $owned_getter_thunk$(self.raw_msg()) };
                  let inner = unsafe { $pbr$::InnerProtoString::from_raw($pbi$::Private, owned) };

                  $transform_owned$
                )rs");
          } else {
            ctx.Emit(R"rs(
                let view = unsafe { $getter_thunk$(self.raw_msg()).as_ref() };
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
            ctx.Emit({},
                     R"rs(
              let s = val.into_proxied($pbi$::Private);
              unsafe {
                $setter_thunk$(
                  self.as_mutator_message_ref($pbi$::Private).msg(),
                  s.into_inner($pbi$::Private).into_raw($pbi$::Private)
                );
              }
            )rs");
          } else {
            ctx.Emit(R"rs(
              let s = val.into_proxied($pbi$::Private);
              let (view, arena) =
                s.into_inner($pbi$::Private).into_raw_parts($pbi$::Private);

              let mm_ref =
                self.as_mutator_message_ref($pbi$::Private);
              let parent_arena = mm_ref.arena($pbi$::Private);

              parent_arena.fuse(&arena);

              unsafe {
                $setter_thunk$(
                  self.as_mutator_message_ref($pbi$::Private).msg(),
                  view
                );
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
        }},
       {"hazzer",
        [&] {
          if (!field.has_presence()) return;
          ctx.Emit({}, R"rs(
                pub fn has_$raw_field_name$($view_self$) -> bool {
                  unsafe { $hazzer_thunk$(self.raw_msg()) }
                })rs");
        }},
       {"clearer",
        [&] {
          if (accessor_case == AccessorCase::VIEW) return;
          if (!field.has_presence()) return;
          ctx.Emit({}, R"rs(
                pub fn clear_$raw_field_name$(&mut self) {
                  unsafe { $clearer_thunk$(self.raw_msg()) }
                })rs");
        }}},
      R"rs(
        $getter$
        $setter$
        $hazzer$
        $clearer$
      )rs");
}

void SingularCord::InExternC(Context& ctx, const FieldDescriptor& field) const {
  ctx.Emit(
      {{"hazzer_thunk", ThunkName(ctx, field, "has")},
       {"borrowed_getter_thunk", ThunkName(ctx, field, "get_cord_borrowed")},
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
       {"clearer_thunk", ThunkName(ctx, field, "clear")},
       {"getter_thunks",
        [&] {
          if (ctx.is_cpp()) {
            ctx.Emit(R"rs(
             fn $is_flat_thunk$(raw_msg: $pbr$::RawMessage) -> bool;
             fn $borrowed_getter_thunk$(raw_msg: $pbr$::RawMessage) -> $pbr$::PtrAndLen;
             fn $owned_getter_thunk$(raw_msg: $pbr$::RawMessage) -> $pbr$::CppStdString;
           )rs");
          } else {
            ctx.Emit(R"rs(
             fn $getter_thunk$(raw_msg: $pbr$::RawMessage) -> $pbr$::PtrAndLen;
           )rs");
          }
        }},
       {"with_presence_fields_thunks",
        [&] {
          if (field.has_presence()) {
            ctx.Emit(R"rs(
                     fn $hazzer_thunk$(raw_msg: $pbr$::RawMessage) -> bool;
                     fn $clearer_thunk$(raw_msg: $pbr$::RawMessage);
                   )rs");
          }
        }}},
      R"rs(
          $with_presence_fields_thunks$
          $getter_thunks$
          $setter$
        )rs");
}

void SingularCord::InThunkCc(Context& ctx, const FieldDescriptor& field) const {
  ctx.Emit(
      {{"field", cpp::FieldName(&field)},
       {"QualifiedMsg", cpp::QualifiedClassName(field.containing_type())},
       {"hazzer_thunk", ThunkName(ctx, field, "has")},
       {"setter_thunk", ThunkName(ctx, field, "set")},
       {"clearer_thunk", ThunkName(ctx, field, "clear")},
       {"borrowed_getter_thunk", ThunkName(ctx, field, "get_cord_borrowed")},
       {"owned_getter_thunk", ThunkName(ctx, field, "get_cord_owned")},
       {"is_flat_thunk", ThunkName(ctx, field, "cord_is_flat")},
       {"with_presence_fields_thunks",
        [&] {
          if (field.has_presence()) {
            ctx.Emit(R"cc(
              bool $hazzer_thunk$($QualifiedMsg$* msg) {
                return msg->has_$field$();
              }
              void $clearer_thunk$($QualifiedMsg$* msg) { msg->clear_$field$(); }
            )cc");
          }
        }}},
      R"cc(
        $with_presence_fields_thunks$;

        bool $is_flat_thunk$($QualifiedMsg$* msg) {
          const absl::Cord& cord = msg->$field$();
          return cord.TryFlat().has_value();
        }
        ::google::protobuf::rust::PtrAndLen $borrowed_getter_thunk$($QualifiedMsg$* msg) {
          const absl::Cord& cord = msg->$field$();
          absl::string_view s = cord.TryFlat().value();
          return ::google::protobuf::rust::PtrAndLen(s.data(), s.size());
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
