// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google Inc.  All rights reserved.
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
//     * Neither the name of Google Inc. nor the names of its
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

#include <memory>

#include "absl/strings/string_view.h"
#include "google/protobuf/compiler/cpp/helpers.h"
#include "google/protobuf/compiler/rust/accessors/accessors.h"
#include "google/protobuf/compiler/rust/context.h"
#include "google/protobuf/compiler/rust/naming.h"
#include "google/protobuf/descriptor.h"

namespace google {
namespace protobuf {
namespace compiler {
namespace rust {
namespace {
class SingularScalar final : public AccessorGenerator {
 public:
  ~SingularScalar() override = default;

  void InMsgImpl(Context<FieldDescriptor> field) const override {
    field.Emit(
        {
            {"field", field.desc().name()},
            {"Scalar", PrimitiveRsTypeName(field)},
            {"hazzer_thunk", Thunk(field, "has")},
            {"getter_thunk", Thunk(field, "get")},
            {"setter_thunk", Thunk(field, "set")},
            {"clearer_thunk", Thunk(field, "clear")},
        },
        R"rs(
          pub fn $field$(&self) -> Option<$Scalar$> {
            if !unsafe { $hazzer_thunk$(self.msg) } {
              return None;
            }
            Some(unsafe { $getter_thunk$(self.msg) })
          }
          pub fn $field$_set(&mut self, val: Option<$Scalar$>) {
            match val {
              Some(val) => unsafe { $setter_thunk$(self.msg, val) },
              None => unsafe { $clearer_thunk$(self.msg) },
            }
          }
        )rs");
  }

  void InExternC(Context<FieldDescriptor> field) const override {
    field.Emit(
        {
            {"Scalar", PrimitiveRsTypeName(field)},
            {"hazzer_thunk", Thunk(field, "has")},
            {"getter_thunk", Thunk(field, "get")},
            {"setter_thunk", Thunk(field, "set")},
            {"clearer_thunk", Thunk(field, "clear")},
        },
        R"rs(
          fn $hazzer_thunk$(raw_msg: $NonNull$<u8>) -> bool;
          fn $getter_thunk$(raw_msg: $NonNull$<u8>) -> $Scalar$;
          fn $setter_thunk$(raw_msg: $NonNull$<u8>, val: $Scalar$);
          fn $clearer_thunk$(raw_msg: $NonNull$<u8>);
        )rs");
  }

  void InThunkCc(Context<FieldDescriptor> field) const override {
    field.Emit(
        {
            {"field", field.desc().name()},
            {"Scalar", cpp::PrimitiveTypeName(field.desc().cpp_type())},
            {"QualifiedMsg",
             cpp::QualifiedClassName(field.desc().containing_type())},
            {"hazzer_thunk", Thunk(field, "has")},
            {"getter_thunk", Thunk(field, "get")},
            {"setter_thunk", Thunk(field, "set")},
            {"clearer_thunk", Thunk(field, "clear")},
        },
        R"cc(
          bool $hazzer_thunk$($QualifiedMsg$* msg) {
            return msg->has_$field$();
          }
          $Scalar$ $getter_thunk$($QualifiedMsg$* msg) { return msg->$field$(); }
          void $setter_thunk$($QualifiedMsg$* msg, $Scalar$ val) {
            msg->set_$field$(val);
          }
          void $clearer_thunk$($QualifiedMsg$* msg) { msg->clear_$field$(); }
        )cc");
  }
};
}  // namespace

std::unique_ptr<AccessorGenerator> AccessorGenerator::ForSingularScalar(
    Context<FieldDescriptor> field) {
  return std::make_unique<SingularScalar>();
}
}  // namespace rust
}  // namespace compiler
}  // namespace protobuf
}  // namespace google
