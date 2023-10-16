// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "absl/strings/match.h"
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
  // here we defer unit tests with messages that have import inside their
  // pkg name e.g. unittest_import.proto
  if (absl::StrContains(prefix, "import")) {
    // TODO: Handle imports correctly, default to $Msg$View for now
    prefix = field.desc().containing_type()->name();
  }
  if (field.is_cpp()) {
    field.Emit({{"prefix", prefix},
                {"field", field.desc().name()},
                {"getter_thunk", Thunk(field, "get")}},
               R"rs(
          pub fn r#$field$(&self) -> $prefix$View {
            // For C++ kernel, getters automatically return the
            // default_instance if the field is unset.
            let submsg = unsafe { $getter_thunk$(self.inner.msg) };
            $prefix$View::new($pbi$::Private, submsg)
          }
        )rs");
  } else {
    field.Emit({{"prefix", prefix},
                {"field", field.desc().name()},
                {"getter_thunk", Thunk(field, "get")}},
               R"rs(
          pub fn r#$field$(&self) -> $prefix$View {
            let submsg = unsafe { $getter_thunk$(self.inner.msg) };
            // For upb, getters return null if the field is unset, so we need to
            // check for null and return the default instance manually. Note that
            // a null ptr received from upb manifests as Option::None
            match submsg {
                // TODO:(b/304357029)
                None => $prefix$View::new($pbi$::Private, $pbr$::ScratchSpace::zeroed_block()),
                Some(field) => $prefix$View::new($pbi$::Private, field),
              }
          }
        )rs");
  }
}

void SingularMessage::InExternC(Context<FieldDescriptor> field) const {
  field.Emit(
      {
          {"getter_thunk", Thunk(field, "get")},
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
               )rs");
}

void SingularMessage::InThunkCc(Context<FieldDescriptor> field) const {
  field.Emit({{"QualifiedMsg",
               cpp::QualifiedClassName(field.desc().containing_type())},
              {"getter_thunk", Thunk(field, "get")},
              {"field", cpp::FieldName(&field.desc())}},
             R"cc(
               const void* $getter_thunk$($QualifiedMsg$* msg) {
                 return static_cast<const void*>(&msg->$field$());
               }
             )cc");
}

}  // namespace rust
}  // namespace compiler
}  // namespace protobuf
}  // namespace google
