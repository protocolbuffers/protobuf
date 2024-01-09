// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include <string>

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

void SingularMessage::InMsgImpl(Context& ctx,
                                const FieldDescriptor& field) const {
  std::string prefix = RsTypePath(ctx, field);

  ctx.Emit(
      {
          {"prefix", prefix},
          {"field", field.name()},
          {"getter_thunk", ThunkName(ctx, field, "get")},
          {"getter_mut_thunk", ThunkName(ctx, field, "get_mut")},
          {"clearer_thunk", ThunkName(ctx, field, "clear")},
          {
              "view_body",
              [&] {
                if (ctx.is_upb()) {
                  ctx.Emit({}, R"rs(
              let submsg = unsafe { $getter_thunk$(self.inner.msg) };
              // For upb, getters return null if the field is unset, so we need
              // to check for null and return the default instance manually.
              // Note that a nullptr received from upb manifests as Option::None
              match submsg {
                // TODO:(b/304357029)
                None => $prefix$View::new($pbi$::Private,
                        $pbr$::ScratchSpace::zeroed_block($pbi$::Private)),
                Some(field) => $prefix$View::new($pbi$::Private, field),
              }
        )rs");
                } else {
                  ctx.Emit({}, R"rs(
              // For C++ kernel, getters automatically return the
              // default_instance if the field is unset.
              let submsg = unsafe { $getter_thunk$(self.inner.msg) };
              $prefix$View::new($pbi$::Private, submsg)
        )rs");
                }
              },
          },
          {"submessage_mut",
           [&] {
             if (ctx.is_upb()) {
               ctx.Emit({}, R"rs(
                 let submsg = unsafe {
                   $getter_mut_thunk$(self.inner.msg, self.inner.arena.raw())
                 };
                 $prefix$Mut::new($pbi$::Private, &mut self.inner, submsg)
                 )rs");
             } else {
               ctx.Emit({}, R"rs(
                    let submsg = unsafe { $getter_mut_thunk$(self.inner.msg) };
                    $prefix$Mut::new($pbi$::Private, &mut self.inner, submsg)
                  )rs");
             }
           }},
      },
      R"rs(
            pub fn r#$field$(&self) -> $prefix$View {
              $view_body$
            }

            pub fn $field$_mut(&mut self) -> $prefix$Mut {
              $submessage_mut$
            }

            pub fn $field$_clear(&mut self) {
              unsafe { $clearer_thunk$(self.inner.msg) }
            }
        )rs");
}

void SingularMessage::InExternC(Context& ctx,
                                const FieldDescriptor& field) const {
  ctx.Emit(
      {
          {"getter_thunk", ThunkName(ctx, field, "get")},
          {"getter_mut_thunk", ThunkName(ctx, field, "get_mut")},
          {"clearer_thunk", ThunkName(ctx, field, "clear")},
          {"getter_mut",
           [&] {
             if (ctx.is_cpp()) {
               ctx.Emit(
                   R"rs(
                    fn $getter_mut_thunk$(raw_msg: $pbi$::RawMessage)
                       -> $pbi$::RawMessage;)rs");
             } else {
               ctx.Emit(
                   R"rs(fn $getter_mut_thunk$(raw_msg: $pbi$::RawMessage,
                                               arena: $pbi$::RawArena)
                            -> $pbi$::RawMessage;)rs");
             }
           }},
          {"ReturnType",
           [&] {
             if (ctx.is_cpp()) {
               // guaranteed to have a nonnull submsg for the cpp kernel
               ctx.Emit({}, "$pbi$::RawMessage;");
             } else {
               // upb kernel may return NULL for a submsg, we can detect this
               // in terra rust if the option returned is None
               ctx.Emit({}, "Option<$pbi$::RawMessage>;");
             }
           }},
      },
      R"rs(
                  fn $getter_thunk$(raw_msg: $pbi$::RawMessage) -> $ReturnType$;
                  $getter_mut$
                  fn $clearer_thunk$(raw_msg: $pbi$::RawMessage);
               )rs");
}

void SingularMessage::InThunkCc(Context& ctx,
                                const FieldDescriptor& field) const {
  ctx.Emit({{"QualifiedMsg", cpp::QualifiedClassName(field.containing_type())},
            {"getter_thunk", ThunkName(ctx, field, "get")},
            {"getter_mut_thunk", ThunkName(ctx, field, "get_mut")},
            {"clearer_thunk", ThunkName(ctx, field, "clear")},
            {"field", cpp::FieldName(&field)}},
           R"cc(
             const void* $getter_thunk$($QualifiedMsg$* msg) {
               return static_cast<const void*>(&msg->$field$());
             }
             void* $getter_mut_thunk$($QualifiedMsg$* msg) {
               return static_cast<void*>(msg->mutable_$field$());
             }
             void $clearer_thunk$($QualifiedMsg$* msg) { msg->clear_$field$(); }
           )cc");
}

}  // namespace rust
}  // namespace compiler
}  // namespace protobuf
}  // namespace google
