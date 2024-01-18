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
#include "google/protobuf/compiler/rust/accessors/accessor_generator.h"
#include "google/protobuf/compiler/rust/accessors/helpers.h"
#include "google/protobuf/compiler/rust/context.h"
#include "google/protobuf/compiler/rust/naming.h"
#include "google/protobuf/descriptor.h"

namespace google {
namespace protobuf {
namespace compiler {
namespace rust {

void SingularString::InMsgImpl(Context& ctx, const FieldDescriptor& field,
                               AccessorCase accessor_case) const {
  ctx.Emit(
      {
          {"field", RsSafeName(field.name())},
          {"hazzer_thunk", ThunkName(ctx, field, "has")},
          {"getter_thunk", ThunkName(ctx, field, "get")},
          {"setter_thunk", ThunkName(ctx, field, "set")},
          {"clearer_thunk", ThunkName(ctx, field, "clear")},
          {"proxied_type", RsTypePath(ctx, field)},
          {"transform_view",
           [&] {
             if (field.type() == FieldDescriptor::TYPE_STRING) {
               ctx.Emit(R"rs(
                // SAFETY: The runtime doesn't require ProtoStr to be UTF-8.
                unsafe { $pb$::ProtoStr::from_utf8_unchecked(view) }
              )rs");
             } else {
               ctx.Emit("view");
             }
           }},
          {"view_lifetime", ViewLifetime(accessor_case)},
          {"view_self", ViewReceiver(accessor_case)},
          {"field_optional_getter",
           [&] {
             if (!field.is_optional()) return;
             if (!field.has_presence()) return;
             ctx.Emit(R"rs(
            pub fn $field$_opt($view_self$) -> $pb$::Optional<&$view_lifetime$ $proxied_type$> {
                let view = unsafe { $getter_thunk$(self.raw_msg()).as_ref() };
                $pb$::Optional::new(
                  $transform_view$ ,
                  unsafe { $hazzer_thunk$(self.raw_msg()) }
                )
              }
          )rs");
           }},
          {"field_mutator_getter",
           [&] {
             if (accessor_case == AccessorCase::VIEW) {
               return;
             }
             if (field.has_presence()) {
               ctx.Emit(
                   {
                       {"default_val", DefaultValue(ctx, field)},
                       {"transform_field_entry",
                        [&] {
                          if (field.type() == FieldDescriptor::TYPE_STRING) {
                            ctx.Emit(R"rs(
                              $pb$::ProtoStrMut::field_entry_from_bytes(
                                $pbi$::Private, out
                              )
                            )rs");
                          } else {
                            ctx.Emit("out");
                          }
                        }},
                   },
                   R"rs(
            pub fn $field$_mut(&mut self) -> $pb$::FieldEntry<'_, $proxied_type$> {
              static VTABLE: $pbi$::BytesOptionalMutVTable = unsafe {
                $pbi$::BytesOptionalMutVTable::new(
                  $pbi$::Private,
                  $getter_thunk$,
                  $setter_thunk$,
                  $clearer_thunk$,
                  $default_val$,
                )
              };
              let out = unsafe {
                let has = $hazzer_thunk$(self.raw_msg());
                $pbi$::new_vtable_field_entry(
                  $pbi$::Private,
                  self.as_mutator_message_ref(),
                  &VTABLE,
                  has,
                )
              };
              $transform_field_entry$
            }
          )rs");
             } else {
               ctx.Emit(R"rs(
              pub fn $field$_mut(&mut self) -> $pb$::Mut<'_, $proxied_type$> {
                static VTABLE: $pbi$::BytesMutVTable =
                  $pbi$::BytesMutVTable::new(
                    $pbi$::Private,
                    $getter_thunk$,
                    $setter_thunk$,
                  );
                unsafe {
                  <$pb$::Mut<$proxied_type$>>::from_inner(
                    $pbi$::Private,
                    $pbi$::RawVTableMutator::new(
                      $pbi$::Private,
                      self.as_mutator_message_ref(),
                      &VTABLE,
                    )
                  )
                }
              }
            )rs");
             }
           }},
      },
      R"rs(
        pub fn $field$($view_self$) -> &$view_lifetime$ $proxied_type$ {
          let view = unsafe { $getter_thunk$(self.raw_msg()).as_ref() };
          $transform_view$
        }

        $field_optional_getter$
        $field_mutator_getter$
      )rs");
}

void SingularString::InExternC(Context& ctx,
                               const FieldDescriptor& field) const {
  ctx.Emit({{"hazzer_thunk", ThunkName(ctx, field, "has")},
            {"getter_thunk", ThunkName(ctx, field, "get")},
            {"setter_thunk", ThunkName(ctx, field, "set")},
            {"clearer_thunk", ThunkName(ctx, field, "clear")},
            {"with_presence_fields_thunks",
             [&] {
               if (field.has_presence()) {
                 ctx.Emit(R"rs(
                     fn $hazzer_thunk$(raw_msg: $pbi$::RawMessage) -> bool;
                     fn $clearer_thunk$(raw_msg: $pbi$::RawMessage);
                   )rs");
               }
             }}},
           R"rs(
          $with_presence_fields_thunks$
          fn $getter_thunk$(raw_msg: $pbi$::RawMessage) -> $pbi$::PtrAndLen;
          fn $setter_thunk$(raw_msg: $pbi$::RawMessage, val: $pbi$::PtrAndLen);
        )rs");
}

void SingularString::InThunkCc(Context& ctx,
                               const FieldDescriptor& field) const {
  ctx.Emit({{"field", cpp::FieldName(&field)},
            {"QualifiedMsg", cpp::QualifiedClassName(field.containing_type())},
            {"hazzer_thunk", ThunkName(ctx, field, "has")},
            {"getter_thunk", ThunkName(ctx, field, "get")},
            {"setter_thunk", ThunkName(ctx, field, "set")},
            {"clearer_thunk", ThunkName(ctx, field, "clear")},
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
             ::google::protobuf::rust_internal::PtrAndLen $getter_thunk$($QualifiedMsg$* msg) {
               absl::string_view val = msg->$field$();
               return ::google::protobuf::rust_internal::PtrAndLen(val.data(), val.size());
             }
             void $setter_thunk$($QualifiedMsg$* msg, ::google::protobuf::rust_internal::PtrAndLen s) {
               msg->set_$field$(absl::string_view(s.ptr, s.len));
             }
           )cc");
}

}  // namespace rust
}  // namespace compiler
}  // namespace protobuf
}  // namespace google
