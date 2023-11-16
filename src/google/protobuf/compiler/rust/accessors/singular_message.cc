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

void SingularMessage::InMsgImpl(Context<FieldDescriptor> field) const {
  Context<Descriptor> d = field.WithDesc(field.desc().message_type());

  auto prefix = "crate::" + GetCrateRelativeQualifiedPath(d);

  field.Emit(
      {
          {"prefix", prefix},
          {"field", field.desc().name()},
          {"getter_thunk", Thunk(field, "get")},
          {"getter_mut_thunk", Thunk(field, "get_mut")},
          {"clearer_thunk", Thunk(field, "clear")},
          {
              "view_body",
              [&] {
                if (field.is_upb()) {
                  field.Emit({}, R"rs(
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
                  field.Emit({}, R"rs(
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
             if (field.is_upb()) {
               field.Emit({}, R"rs(
                 let submsg_opt = unsafe { $getter_thunk$(self.inner.msg) };
                 match submsg_opt {
                   None => {
                     $prefix$Mut::new($pbi$::Private,
                       &mut self.inner,
                       $pbr$::ScratchSpace::zeroed_block($pbi$::Private))
                   },
                   Some(submsg) => {
                     $prefix$Mut::new($pbi$::Private, &mut self.inner, submsg)
                }
              }
        )rs");
             } else {
               field.Emit({}, R"rs(
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

void SingularMessage::InExternC(Context<FieldDescriptor> field) const {
  field.Emit(
      {
          {"getter_thunk", Thunk(field, "get")},
          {"getter_mut_thunk", Thunk(field, "get_mut")},
          {"clearer_thunk", Thunk(field, "clear")},
          {"ReturnType",
           [&] {
             if (field.is_cpp()) {
               // guaranteed to have a nonnull submsg for the cpp kernel
               field.Emit({}, "$pbi$::RawMessage;");
             } else {
               // upb kernel may return NULL for a submsg, we can detect this
               // in terra rust if the option returned is None
               field.Emit({}, "Option<$pbi$::RawMessage>;");
             }
           }},
      },
      R"rs(
                  fn $getter_thunk$(raw_msg: $pbi$::RawMessage) -> $ReturnType$;
                  fn $getter_mut_thunk$(raw_msg: $pbi$::RawMessage) -> $ReturnType$;
                  fn $clearer_thunk$(raw_msg: $pbi$::RawMessage);
               )rs");
}

void SingularMessage::InThunkCc(Context<FieldDescriptor> field) const {
  field.Emit({{"QualifiedMsg",
               cpp::QualifiedClassName(field.desc().containing_type())},
              {"getter_thunk", Thunk(field, "get")},
              {"getter_mut_thunk", Thunk(field, "get_mut")},
              {"clearer_thunk", Thunk(field, "clear")},
              {"field", cpp::FieldName(&field.desc())}},
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
