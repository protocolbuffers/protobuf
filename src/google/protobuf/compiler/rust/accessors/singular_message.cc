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

#include <memory>

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
class SingularMessage final : public AccessorGenerator {
 public:
  ~SingularMessage() override = default;

  void InMsgImpl(Context<FieldDescriptor> field) const override {
    field.Emit({{"rsclass", GetRsQualifiedName(field)},
                {"field", field.desc().name()},
                {"QualifiedMsg",
                 cpp::QualifiedClassName(field.desc().containing_type())},
                {"getter_thunk", Thunk(field, "get")},
                {"getter",
                 [&] {
                   field.Emit({}, R"rs(
                                    pub fn $field$(&self) -> $rsclass$ {
                                      unsafe { $getter_thunk$(self.msg) }
                                    }
                                  )rs");
                 }}},
               R"rs(
               $getter$
                )rs");
  }

  void InExternC(Context<FieldDescriptor> field) const override {
    field.Emit(
        {
            {"rsclass", GetRsQualifiedName(field)},
            {"field", field.desc().name()},
            {"QualifiedMsg",
             cpp::QualifiedClassName(field.desc().containing_type())},
            {"getter_thunk", Thunk(field, "get")},
        },
        R"rs(
          fn $getter_thunk$(raw_msg: $NonNull$<u8>) -> $rsclass$;
        )rs");
  }

  void InThunkCc(Context<FieldDescriptor> field) const override {
    field.Emit(
        {
            {"cppclass", cpp::QualifiedClassName(field.desc().message_type())},
            {"QualifiedMsg",
             cpp::QualifiedClassName(field.desc().containing_type())},
            {"field", field.desc().name()},
            {"getter_thunk", Thunk(field, "get")},
        },
        R"cc(
          // todo: cpp hand-written thunks
        )cc");
  }
};
}  // namespace

std::unique_ptr<AccessorGenerator> AccessorGenerator::ForSingularMessage(
    Context<FieldDescriptor> field) {
  return std::make_unique<SingularMessage>();
}
}  // namespace rust
}  // namespace compiler
}  // namespace protobuf
}  // namespace google
