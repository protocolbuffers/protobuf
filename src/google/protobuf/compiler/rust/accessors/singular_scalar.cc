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

void SingularScalar::InMsgImpl(Context<FieldDescriptor> field) const {
  field.Emit(
      {
          {"field", field.desc().name()},
          {"Scalar", PrimitiveRsTypeName(field.desc())},
          {"hazzer_thunk", Thunk(field, "has")},
          {"getter",
           [&] {
             field.Emit({}, R"rs(
                  pub fn r#$field$(&self) -> $Scalar$ {
                    unsafe { $getter_thunk$(self.inner.msg) }
                  }
                )rs");
           }},
          {"getter_opt",
           [&] {
             if (!field.desc().is_optional()) return;
             if (!field.desc().has_presence()) return;
             // TODO(b/285309449): use Optional instead of Option
             field.Emit({}, R"rs(
                  pub fn r#$field$_opt(&self) -> Option<$Scalar$> {
                    if !unsafe { $hazzer_thunk$(self.inner.msg) } {
                      return None;
                    }
                    Some(unsafe { $getter_thunk$(self.inner.msg) })
                  }
                  )rs");
           }},
          {"getter_thunk", Thunk(field, "get")},
          {"setter_thunk", Thunk(field, "set")},
          {"clearer_thunk", Thunk(field, "clear")},
          {"field_setter",
           [&] {
             if (field.desc().has_presence()) {
               field.Emit({}, R"rs(
                  pub fn r#$field$_set(&mut self, val: Option<$Scalar$>) {
                    match val {
                      Some(val) => unsafe { $setter_thunk$(self.inner.msg, val) },
                      None => unsafe { $clearer_thunk$(self.inner.msg) },
                    }
                  }
                )rs");
             }
           }},
          {"field_mutator_getter",
           [&] {
             if (field.desc().has_presence()) {
               // TODO(b/285309449): implement mutator for fields with presence.
               return;
             } else {
               field.Emit({}, R"rs(
                  pub fn r#$field$_mut(&mut self) -> $pb$::PrimitiveMut<'_, $Scalar$> {
                    static VTABLE: $pbi$::PrimitiveVTable<$Scalar$> =
                      $pbi$::PrimitiveVTable::new(
                        $pbi$::Private,
                        $getter_thunk$,
                        $setter_thunk$,
                      );

                      $pb$::PrimitiveMut::from_inner(
                        $pbi$::Private,
                        unsafe {
                          $pbi$::RawVTableMutator::new(
                            $pbi$::Private,
                            $pbr$::MutatorMessageRef::new(
                              $pbi$::Private, &mut self.inner
                            ),
                            &VTABLE,
                          )
                        },
                      )
                  }
                )rs");
             }
           }},
      },
      R"rs(
          $getter$
          $getter_opt$
          $field_setter$
          $field_mutator_getter$
        )rs");
}

void SingularScalar::InExternC(Context<FieldDescriptor> field) const {
  field.Emit(
      {{"Scalar", PrimitiveRsTypeName(field.desc())},
       {"hazzer_thunk", Thunk(field, "has")},
       {"getter_thunk", Thunk(field, "get")},
       {"setter_thunk", Thunk(field, "set")},
       {"clearer_thunk", Thunk(field, "clear")},
       {"hazzer",
        [&] {
          if (field.desc().has_presence()) {
            field.Emit(
                R"rs(fn $hazzer_thunk$(raw_msg: $pbi$::RawMessage) -> bool;)rs");
          }
        }}},
      R"rs(
          $hazzer$
          fn $getter_thunk$(raw_msg: $pbi$::RawMessage) -> $Scalar$;
          fn $setter_thunk$(raw_msg: $pbi$::RawMessage, val: $Scalar$);
          fn $clearer_thunk$(raw_msg: $pbi$::RawMessage);
        )rs");
}

void SingularScalar::InThunkCc(Context<FieldDescriptor> field) const {
  field.Emit({{"field", cpp::FieldName(&field.desc())},
              {"Scalar", cpp::PrimitiveTypeName(field.desc().cpp_type())},
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
                   )cc");
                 }
               }}},
             R"cc(
               $hazzer$;
               $Scalar$ $getter_thunk$($QualifiedMsg$* msg) { return msg->$field$(); }
               void $setter_thunk$($QualifiedMsg$* msg, $Scalar$ val) {
                 msg->set_$field$(val);
               }
               void $clearer_thunk$($QualifiedMsg$* msg) { msg->clear_$field$(); }
             )cc");
}

}  // namespace rust
}  // namespace compiler
}  // namespace protobuf
}  // namespace google
