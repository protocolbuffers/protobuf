// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
// https://developers.google.com/protocol-buffers/
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
//     * Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above
// copyright notice, this list of conditions and the following disclaimer
// in the documentation and/or other materials provided with the
// distribution.
//     * Neither the name of Google LLC. nor the names of its
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#include <string>

#include "absl/strings/escaping.h"
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

void SingularString::InMsgImpl(Context<FieldDescriptor> field) const {
  std::string hazzer_thunk = Thunk(field, "has");
  std::string getter_thunk = Thunk(field, "get");
  std::string setter_thunk = Thunk(field, "set");
  std::string proxied_type = PrimitiveRsTypeName(field.desc());
  auto transform_view = [&] {
    if (field.desc().type() == FieldDescriptor::TYPE_STRING) {
      field.Emit(R"rs(
        // SAFETY: The runtime doesn't require ProtoStr to be UTF-8.
        unsafe { $pb$::ProtoStr::from_utf8_unchecked(view) }
      )rs");
    } else {
      field.Emit("view");
    }
  };
  field.Emit(
      {
          {"field", field.desc().name()},
          {"hazzer_thunk", hazzer_thunk},
          {"getter_thunk", getter_thunk},
          {"setter_thunk", setter_thunk},
          {"proxied_type", proxied_type},
          {"transform_view", transform_view},
          {"field_optional_getter",
           [&] {
             if (!field.desc().is_optional()) return;
             if (!field.desc().has_presence()) return;
             field.Emit({{"hazzer_thunk", hazzer_thunk},
                         {"getter_thunk", getter_thunk},
                         {"transform_view", transform_view}},
                        R"rs(
            pub fn $field$_opt(&self) -> $pb$::Optional<&$proxied_type$> {
              unsafe {
                let view = $getter_thunk$(self.inner.msg).as_ref();
                $pb$::Optional::new(
                  $transform_view$ ,
                  $hazzer_thunk$(self.inner.msg)
                )
              }
            }
          )rs");
           }},
          {"field_mutator_getter",
           [&] {
             if (field.desc().has_presence()) {
               field.Emit(
                   {
                       {"field", field.desc().name()},
                       {"proxied_type", proxied_type},
                       {"default_val",
                        absl::CHexEscape(field.desc().default_value_string())},
                       {"view_type", proxied_type},
                       {"transform_field_entry",
                        [&] {
                          if (field.desc().type() ==
                              FieldDescriptor::TYPE_STRING) {
                            field.Emit(R"rs(
                              $pb$::ProtoStrMut::field_entry_from_bytes(
                                $pbi$::Private, out
                              )
                            )rs");
                          } else {
                            field.Emit("out");
                          }
                        }},
                       {"hazzer_thunk", hazzer_thunk},
                       {"getter_thunk", getter_thunk},
                       {"setter_thunk", setter_thunk},
                       {"clearer_thunk", Thunk(field, "clear")},
                   },
                   R"rs(
            pub fn $field$_mut(&mut self) -> $pb$::FieldEntry<'_, $proxied_type$> {
              static VTABLE: $pbi$::BytesOptionalMutVTable = unsafe {
                $pbi$::BytesOptionalMutVTable::new(
                  $pbi$::Private,
                  $getter_thunk$,
                  $setter_thunk$,
                  $clearer_thunk$,
                  b"$default_val$",
                )
              };
              let out = unsafe {
                let has = $hazzer_thunk$(self.inner.msg);
                $pbi$::new_vtable_field_entry(
                  $pbi$::Private,
                  $pbr$::MutatorMessageRef::new(
                    $pbi$::Private, &mut self.inner),
                  &VTABLE,
                  has,
                )
              };
              $transform_field_entry$
            }
          )rs");
             } else {
               field.Emit({{"field", field.desc().name()},
                           {"proxied_type", proxied_type},
                           {"getter_thunk", getter_thunk},
                           {"setter_thunk", setter_thunk}},
                          R"rs(
              pub fn $field$_mut(&mut self) -> $pb$::Mut<'_, $proxied_type$> {
                static VTABLE: $pbi$::BytesMutVTable = unsafe {
                  $pbi$::BytesMutVTable::new(
                    $pbi$::Private,
                    $getter_thunk$,
                    $setter_thunk$,
                  )
                };
                unsafe {
                  <$pb$::Mut<$proxied_type$>>::from_inner(
                    $pbi$::Private,
                    $pbi$::RawVTableMutator::new(
                      $pbi$::Private,
                      $pbr$::MutatorMessageRef::new(
                        $pbi$::Private, &mut self.inner),
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
        pub fn r#$field$(&self) -> &$proxied_type$ {
          let view = unsafe { $getter_thunk$(self.inner.msg).as_ref() };
          $transform_view$
        }

        $field_optional_getter$
        $field_mutator_getter$
      )rs");
}

void SingularString::InExternC(Context<FieldDescriptor> field) const {
  field.Emit({{"hazzer_thunk", Thunk(field, "has")},
              {"getter_thunk", Thunk(field, "get")},
              {"setter_thunk", Thunk(field, "set")},
              {"clearer_thunk", Thunk(field, "clear")},
              {"hazzer",
               [&] {
                 if (field.desc().has_presence()) {
                   field.Emit(R"rs(
                     fn $hazzer_thunk$(raw_msg: $pbi$::RawMessage) -> bool;
                   )rs");
                 }
               }}},
             R"rs(
          $hazzer$
          fn $getter_thunk$(raw_msg: $pbi$::RawMessage) -> $pbi$::PtrAndLen;
          fn $setter_thunk$(raw_msg: $pbi$::RawMessage, val: *const u8, len: usize);
          fn $clearer_thunk$(raw_msg: $pbi$::RawMessage);
        )rs");
}

void SingularString::InThunkCc(Context<FieldDescriptor> field) const {
  field.Emit({{"field", cpp::FieldName(&field.desc())},
              {"QualifiedMsg",
               cpp::QualifiedClassName(field.desc().containing_type())},
              {"hazzer_thunk", Thunk(field, "has")},
              {"getter_thunk", Thunk(field, "get")},
              {"setter_thunk", Thunk(field, "set")},
              {"clearer_thunk", Thunk(field, "clear")},
              {"hazzer",
               [&] {
                 if (field.desc().has_presence()) {
                   field.Emit(R"cc(
                     bool $hazzer_thunk$($QualifiedMsg$* msg) {
                       return msg->has_$field$();
                     }
                     void $clearer_thunk$($QualifiedMsg$* msg) { msg->clear_$field$(); }
                   )cc");
                 }
               }}},
             R"cc(
               $hazzer$;
               ::google::protobuf::rust_internal::PtrAndLen $getter_thunk$($QualifiedMsg$* msg) {
                 absl::string_view val = msg->$field$();
                 return google::protobuf::rust_internal::PtrAndLen(val.data(), val.size());
               }
               void $setter_thunk$($QualifiedMsg$* msg, const char* ptr, ::std::size_t size) {
                 msg->set_$field$(absl::string_view(ptr, size));
               }
             )cc");
}

}  // namespace rust
}  // namespace compiler
}  // namespace protobuf
}  // namespace google
