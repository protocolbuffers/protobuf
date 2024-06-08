// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
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

void SingularMessage::InMsgImpl(Context& ctx, const FieldDescriptor& field,
                                AccessorCase accessor_case) const {
  // fully qualified message name with modules prefixed
  std::string msg_type = RsTypePath(ctx, field);
  std::string field_name = FieldNameWithCollisionAvoidance(field);
  ctx.Emit({{"msg_type", msg_type},
            {"field", RsSafeName(field_name)},
            {"raw_field_name", field_name},
            {"view_lifetime", ViewLifetime(accessor_case)},
            {"view_self", ViewReceiver(accessor_case)},
            {"getter_thunk", ThunkName(ctx, field, "get")},
            {"getter_mut_thunk", ThunkName(ctx, field, "get_mut")},
            {"clearer_thunk", ThunkName(ctx, field, "clear")},
            {"hazzer_thunk", ThunkName(ctx, field, "has")},
            {"set_allocated_thunk", ThunkName(ctx, field, "set")},
            {
                "getter_body",
                [&] {
                  if (ctx.is_upb()) {
                    ctx.Emit({}, R"rs(
              let submsg = unsafe { $getter_thunk$(self.raw_msg()) };
              //~ For upb, getters return null if the field is unset, so we need
              //~ to check for null and return the default instance manually.
              //~ Note that a nullptr received from upb manifests as Option::None
              match submsg {
                //~ TODO:(b/304357029)
                None => $msg_type$View::new($pbi$::Private,
                        $pbr$::ScratchSpace::zeroed_block($pbi$::Private)),
                Some(field) => $msg_type$View::new($pbi$::Private, field),
              }
        )rs");
                  } else {
                    ctx.Emit({}, R"rs(
              //~ For C++ kernel, getters automatically return the
              //~ default_instance if the field is unset.
              let submsg = unsafe { $getter_thunk$(self.raw_msg()) };
              $msg_type$View::new($pbi$::Private, submsg)
        )rs");
                  }
                },
            },
            {"getter",
             [&] {
               ctx.Emit({}, R"rs(
                pub fn $field$($view_self$) -> $msg_type$View<$view_lifetime$> {
                  $getter_body$
                }
              )rs");
             }},
            {"getter_mut_body",
             [&] {
               if (ctx.is_cpp()) {
                 ctx.Emit({}, R"rs(
                  let raw_msg = unsafe { $getter_mut_thunk$(self.raw_msg()) };
                  $msg_type$Mut::from_parent($pbi$::Private,
                  self.as_mutator_message_ref($pbi$::Private), raw_msg)
                 )rs");
               } else {
                 ctx.Emit({}, R"rs(
                  let raw_msg = unsafe { 
                    $getter_mut_thunk$(self.raw_msg(), self.arena().raw())
                  };
                  $msg_type$Mut::from_parent($pbi$::Private,
                    self.as_mutator_message_ref($pbi$::Private), raw_msg)
                )rs");
               }
             }},
            {"getter_mut",
             [&] {
               if (accessor_case == AccessorCase::VIEW) {
                 return;
               }

               ctx.Emit({}, R"rs(
                 pub fn $raw_field_name$_mut(&mut self) -> $msg_type$Mut<'_> {
                    $getter_mut_body$
               })rs");
             }},
            {"getter_opt",
             [&] {
               ctx.Emit({}, R"rs(
                pub fn $raw_field_name$_opt($view_self$) ->
                $pb$::Optional<$msg_type$View<$view_lifetime$>> {
                  let view = self.$field$();
                  $pb$::Optional::new(view, self.has_$raw_field_name$())
            }
            )rs");
             }},
            {"setter_body",
             [&] {
               if (accessor_case == AccessorCase::VIEW) return;
               if (ctx.is_upb()) {
                 ctx.Emit({}, R"rs(
                  // The message and arena are dropped after the setter. The
                  // memory remains allocated as we fuse the arena with the
                  // parent message's arena.
                  let mut msg = val.into($pbi$::Private);
                  self.as_mutator_message_ref($pbi$::Private)
                    .arena($pbi$::Private)
                    .fuse(msg.as_mutator_message_ref($pbi$::Private).arena($pbi$::Private));

                  unsafe {
                    $set_allocated_thunk$(self.as_mutator_message_ref($pbi$::Private).msg(),
                      msg.as_mutator_message_ref($pbi$::Private).msg());
                  }
                )rs");
               } else {
                 ctx.Emit({}, R"rs(
                  // Prevent the memory from being deallocated. The setter
                  // transfers ownership of the memory to the parent message.
                  let mut msg = std::mem::ManuallyDrop::new(val.into($pbi$::Private));
                  unsafe {
                    $set_allocated_thunk$(self.as_mutator_message_ref($pbi$::Private).msg(),
                      msg.as_mutator_message_ref($pbi$::Private).msg());
                  }
                )rs");
               }
             }},
            {"setter",
             [&] {
               if (accessor_case == AccessorCase::VIEW) return;
               ctx.Emit(R"rs(
                pub fn set_$raw_field_name$(&mut self,
                  val: impl $pb$::IntoProxied<$msg_type$>) {

                  $setter_body$
                }
              )rs");
             }},
            {"hazzer",
             [&] {
               ctx.Emit({}, R"rs(
                  pub fn has_$raw_field_name$($view_self$) -> bool {
                    unsafe { $hazzer_thunk$(self.raw_msg()) }
                  })rs");
             }},
            {"clearer",
             [&] {
               if (accessor_case == AccessorCase::VIEW) return;
               ctx.Emit({}, R"rs(
                  pub fn clear_$raw_field_name$(&mut self) {
                    unsafe { $clearer_thunk$(self.raw_msg()) }
                  })rs");
             }}},
           R"rs(
            $getter$
            $getter_mut$
            $getter_opt$
            $setter$
            $hazzer$
            $clearer$
        )rs");
}

void SingularMessage::InExternC(Context& ctx,
                                const FieldDescriptor& field) const {
  ctx.Emit(
      {
          {"getter_thunk", ThunkName(ctx, field, "get")},
          {"getter_mut_thunk", ThunkName(ctx, field, "get_mut")},
          {"clearer_thunk", ThunkName(ctx, field, "clear")},
          {"hazzer_thunk", ThunkName(ctx, field, "has")},
          {"set_allocated_thunk", ThunkName(ctx, field, "set")},
          {"getter_mut",
           [&] {
             if (ctx.is_cpp()) {
               ctx.Emit(
                   R"rs(
                    fn $getter_mut_thunk$(raw_msg: $pbr$::RawMessage)
                       -> $pbr$::RawMessage;)rs");
             } else {
               ctx.Emit(
                   R"rs(fn $getter_mut_thunk$(raw_msg: $pbr$::RawMessage,
                                               arena: $pbr$::RawArena)
                            -> $pbr$::RawMessage;)rs");
             }
           }},
          {"ReturnType",
           [&] {
             if (ctx.is_cpp()) {
               // guaranteed to have a nonnull submsg for the cpp kernel
               ctx.Emit({}, "$pbr$::RawMessage;");
             } else {
               // upb kernel may return NULL for a submsg, we can detect this
               // in terra rust if the option returned is None
               ctx.Emit({}, "Option<$pbr$::RawMessage>;");
             }
           }},
      },
      R"rs(
                  fn $getter_thunk$(raw_msg: $pbr$::RawMessage) -> $ReturnType$;
                  $getter_mut$
                  fn $clearer_thunk$(raw_msg: $pbr$::RawMessage);
                  fn $hazzer_thunk$(raw_msg: $pbr$::RawMessage) -> bool;
                  fn $set_allocated_thunk$(raw_msg: $pbr$::RawMessage,
                                    field_msg: $pbr$::RawMessage);
               )rs");
}

void SingularMessage::InThunkCc(Context& ctx,
                                const FieldDescriptor& field) const {
  ctx.Emit({{"QualifiedMsg", cpp::QualifiedClassName(field.containing_type())},
            {"FieldMsg", cpp::QualifiedClassName(field.message_type())},
            {"set_allocated_thunk", ThunkName(ctx, field, "set")},
            {"getter_thunk", ThunkName(ctx, field, "get")},
            {"getter_mut_thunk", ThunkName(ctx, field, "get_mut")},
            {"clearer_thunk", ThunkName(ctx, field, "clear")},
            {"hazzer_thunk", ThunkName(ctx, field, "has")},
            {"field", cpp::FieldName(&field)}},
           R"cc(
             const void* $getter_thunk$($QualifiedMsg$* msg) {
               return static_cast<const void*>(&msg->$field$());
             }
             void* $getter_mut_thunk$($QualifiedMsg$* msg) {
               return static_cast<void*>(msg->mutable_$field$());
             }
             void $clearer_thunk$($QualifiedMsg$* msg) { msg->clear_$field$(); }
             bool $hazzer_thunk$($QualifiedMsg$* msg) { return msg->has_$field$(); }
             void $set_allocated_thunk$($QualifiedMsg$* msg, $FieldMsg$* sub_msg) {
               msg->set_allocated_$field$(sub_msg);
             }
           )cc");
}

}  // namespace rust
}  // namespace compiler
}  // namespace protobuf
}  // namespace google
