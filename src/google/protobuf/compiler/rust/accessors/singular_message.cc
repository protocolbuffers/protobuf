// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "absl/strings/match.h"
#include "absl/strings/string_view.h"
#include "absl/strings/strip.h"
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
  field.Emit(
      {{"prefix", prefix},
       {"field", field.desc().name()},
       {"getter_thunk", Thunk(field, "get")},
       {"getter_new_thunk", Thunk(field, "get_new")},
       {"create_view",
        [&] {
          if (field.is_cpp()) {
            field.Emit({}, R"rs(unreachable!(), )rs");
          } else {
            field.Emit(
                {},
                R"rs($prefix$View::new($pbi$::Private, unsafe { $getter_new_thunk$(self.inner.arena.raw) }),)rs");
          }
        }}},
      R"rs(
          pub fn r#$field$(&self) -> $prefix$View {
            let fetched_field = unsafe { $getter_thunk$(self.inner.msg) };
            match fetched_field {
                None => $create_view$,
                Some(field) => $prefix$View::new($pbi$::Private, field),
              }
          }
        )rs");
}

void SingularMessage::InExternC(Context<FieldDescriptor> field) const {
  field.Emit(
      {
          {"getter_thunk", Thunk(field, "get")},
      },
      R"rs(
                  fn $getter_thunk$(raw_msg: $pbi$::RawMessage) -> Option<$pbi$::RawMessage>;
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
